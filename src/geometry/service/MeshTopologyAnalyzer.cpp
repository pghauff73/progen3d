#include "geometry/service/MeshTopologyAnalyzer.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <tuple>

namespace {

constexpr double kPositionQuantization = 1000000.0;

struct QuantizedPosition
{
	std::int64_t x = 0;
	std::int64_t y = 0;
	std::int64_t z = 0;

	bool operator<(const QuantizedPosition &other) const
	{
		return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
	}

	bool operator==(const QuantizedPosition &other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}
};

struct QuantizedEdge
{
	QuantizedPosition first;
	QuantizedPosition second;

	bool operator<(const QuantizedEdge &other) const
	{
		return std::tie(first, second) < std::tie(other.first, other.second);
	}
};

QuantizedPosition quantize(const glm::vec3 &position, double characteristic_length)
{
	return {
		static_cast<std::int64_t>(std::llround(position.x * kPositionQuantization / characteristic_length)),
		static_cast<std::int64_t>(std::llround(position.y * kPositionQuantization / characteristic_length)),
		static_cast<std::int64_t>(std::llround(position.z * kPositionQuantization / characteristic_length))};
}

QuantizedEdge edge_for(const glm::vec3 &a, const glm::vec3 &b, double characteristic_length)
{
	QuantizedPosition first = quantize(a, characteristic_length);
	QuantizedPosition second = quantize(b, characteristic_length);
	if (second < first) std::swap(first, second);
	return {first, second};
}

void hash_bytes(std::uint64_t *hash, const void *data, std::size_t size)
{
	const auto *bytes = static_cast<const unsigned char *>(data);
	for (std::size_t index = 0; index < size; ++index) {
		*hash ^= bytes[index];
		*hash *= 1099511628211ull;
	}
}

}

MeshTopologyReport MeshTopologyAnalyzer::analyze(
	const Mesh &mesh,
	const std::vector<MeshSurfaceTag> &face_surface_tags,
	float characteristic_length) const
{
	MeshTopologyReport report;
	if (!std::isfinite(characteristic_length) || characteristic_length <= 0.0f) {
		report.degenerate_triangle_count = mesh.faces.size();
		return report;
	}
	std::map<QuantizedEdge, std::size_t> edge_incidents;
	std::uint64_t hash = 1469598103934665603ull;

	for (const glm::vec3 &vertex : mesh.vertices) {
		hash_bytes(&hash, &vertex.x, sizeof(vertex.x));
		hash_bytes(&hash, &vertex.y, sizeof(vertex.y));
		hash_bytes(&hash, &vertex.z, sizeof(vertex.z));
	}

	for (std::size_t face_index = 0; face_index < mesh.faces.size(); ++face_index) {
		const glm::ivec3 &face = mesh.faces[face_index];
		hash_bytes(&hash, &face.x, sizeof(face.x));
		hash_bytes(&hash, &face.y, sizeof(face.y));
		hash_bytes(&hash, &face.z, sizeof(face.z));
		if (face_index < face_surface_tags.size()) {
			const int role = static_cast<int>(face_surface_tags[face_index].role());
			const std::size_t boundary_index =
				face_surface_tags[face_index].boundaryIndex();
			const std::size_t object_part_index =
				face_surface_tags[face_index].objectPartIndex();
			hash_bytes(&hash, &role, sizeof(role));
			hash_bytes(&hash, &boundary_index, sizeof(boundary_index));
			hash_bytes(&hash, &object_part_index, sizeof(object_part_index));
		}

		if (face.x < 0 || face.y < 0 || face.z < 0 ||
		    static_cast<std::size_t>(face.x) >= mesh.vertices.size() ||
		    static_cast<std::size_t>(face.y) >= mesh.vertices.size() ||
		    static_cast<std::size_t>(face.z) >= mesh.vertices.size()) {
			++report.degenerate_triangle_count;
			continue;
		}
		const glm::vec3 &a = mesh.vertices[static_cast<std::size_t>(face.x)];
		const glm::vec3 &b = mesh.vertices[static_cast<std::size_t>(face.y)];
		const glm::vec3 &c = mesh.vertices[static_cast<std::size_t>(face.z)];
		const glm::vec3 cross = glm::cross(b - a, c - a);
		// The default retains the existing unit-scale tolerance. Small anatomical
		// features may supply their model width so valid lid/lip triangles are
		// tested relative to that model, while volume and hashes stay in original units.
		const glm::vec3 relative_cross = cross / (characteristic_length * characteristic_length);
		if (!std::isfinite(cross.x) || !std::isfinite(cross.y) ||
		    !std::isfinite(cross.z) || glm::dot(relative_cross, relative_cross) <= 1.0e-16f) {
			++report.degenerate_triangle_count;
			continue;
		}
		report.signed_volume += glm::dot(a, glm::cross(b, c)) / 6.0f;
		++edge_incidents[edge_for(a, b, characteristic_length)];
		++edge_incidents[edge_for(b, c, characteristic_length)];
		++edge_incidents[edge_for(c, a, characteristic_length)];
	}

	for (const auto &incident : edge_incidents) {
		if (incident.second == 1) ++report.boundary_edge_count;
		else if (incident.second != 2) ++report.nonmanifold_edge_count;
	}
	report.topology_hash = hash;
	return report;
}
