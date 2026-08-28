#include "geometry/service/ShellOffsetGeometryBuilder.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <tuple>
#include <utility>

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

struct BoundaryEdgeRecord
{
	std::size_t incident_count = 0u;
	int start_vertex = -1;
	int end_vertex = -1;
};

QuantizedPosition quantize(const glm::vec3 &position)
{
	return {
		static_cast<std::int64_t>(std::llround(position.x * kPositionQuantization)),
		static_cast<std::int64_t>(std::llround(position.y * kPositionQuantization)),
		static_cast<std::int64_t>(std::llround(position.z * kPositionQuantization))};
}

QuantizedEdge edge_key(const glm::vec3 &first, const glm::vec3 &second)
{
	QuantizedPosition first_key = quantize(first);
	QuantizedPosition second_key = quantize(second);
	if (second_key < first_key) std::swap(first_key, second_key);
	return {first_key, second_key};
}

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

void record_edge(
	const Mesh &mesh,
	int start_vertex,
	int end_vertex,
	std::map<QuantizedEdge, BoundaryEdgeRecord> *edges)
{
	BoundaryEdgeRecord &record = (*edges)[edge_key(
		mesh.vertices[static_cast<std::size_t>(start_vertex)],
		mesh.vertices[static_cast<std::size_t>(end_vertex)])];
	if (record.incident_count == 0u) {
		record.start_vertex = start_vertex;
		record.end_vertex = end_vertex;
	}
	++record.incident_count;
}

}

GeometryBuildResult ShellOffsetGeometryBuilder::build(
	const GeneratedPrimitiveMesh &source,
	const ShellOffsetShapeSpecification &specification) const
{
	if (!source.mesh() || source.mesh()->faces.empty() ||
	    source.faceSurfaceTags().size() != source.mesh()->faces.size()) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			"ShellOffset requires a non-empty source mesh with matching surface tags.");
	}
	if (!std::isfinite(specification.thickness()) ||
	    specification.thickness() <= 0.0f) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::InvalidSpecification,
			"ShellOffset thickness must be finite and positive.");
	}

	const Mesh &source_mesh = *source.mesh();
	std::map<QuantizedPosition, glm::vec3> normal_sums;
	std::map<QuantizedEdge, BoundaryEdgeRecord> edges;
	for (const glm::ivec3 &face : source_mesh.faces) {
		if (face.x < 0 || face.y < 0 || face.z < 0 ||
		    static_cast<std::size_t>(face.x) >= source_mesh.vertices.size() ||
		    static_cast<std::size_t>(face.y) >= source_mesh.vertices.size() ||
		    static_cast<std::size_t>(face.z) >= source_mesh.vertices.size()) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::UnsupportedTopology,
				"ShellOffset source contains an invalid triangle index.");
		}
		const glm::vec3 &first = source_mesh.vertices[static_cast<std::size_t>(face.x)];
		const glm::vec3 &second = source_mesh.vertices[static_cast<std::size_t>(face.y)];
		const glm::vec3 &third = source_mesh.vertices[static_cast<std::size_t>(face.z)];
		const glm::vec3 cross = glm::cross(second - first, third - first);
		if (!finite(cross) || glm::dot(cross, cross) <= 1.0e-16f) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::DegenerateFace,
				"ShellOffset source contains a degenerate triangle.");
		}
		normal_sums[quantize(first)] += cross;
		normal_sums[quantize(second)] += cross;
		normal_sums[quantize(third)] += cross;
		record_edge(source_mesh, face.x, face.y, &edges);
		record_edge(source_mesh, face.y, face.z, &edges);
		record_edge(source_mesh, face.z, face.x, &edges);
	}

	std::size_t boundary_edge_count = 0u;
	for (const auto &edge : edges) {
		if (edge.second.incident_count == 1u) ++boundary_edge_count;
	}
	const std::size_t vertex_count = source_mesh.vertices.size() * 2u;
	const std::size_t triangle_count =
		source_mesh.faces.size() * 2u + boundary_edge_count * 2u;
	if (vertex_count > complexity_limits_.maximumGeneratedVertices() ||
	    triangle_count > complexity_limits_.maximumGeneratedTriangles()) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::TriangleLimitExceeded,
			"ShellOffset exceeds the configured generated mesh limits.");
	}

	float outward_distance = 0.0f;
	float inward_distance = 0.0f;
	if (specification.side() == ShellOffsetSide::Outward) {
		outward_distance = specification.thickness();
	}
	else if (specification.side() == ShellOffsetSide::Inward) {
		inward_distance = specification.thickness();
	}
	else {
		outward_distance = specification.thickness() * 0.5f;
		inward_distance = specification.thickness() * 0.5f;
	}

	auto mesh = std::make_shared<Mesh>();
	mesh->vertices.reserve(vertex_count);
	mesh->normals.reserve(vertex_count);
	mesh->texcoords.reserve(vertex_count);
	mesh->faces.reserve(triangle_count);
	std::vector<MeshSurfaceTag> tags;
	tags.reserve(triangle_count);

	for (std::size_t index = 0; index < source_mesh.vertices.size(); ++index) {
		glm::vec3 normal = normal_sums[quantize(source_mesh.vertices[index])];
		if (glm::dot(normal, normal) <= 1.0e-16f) {
			normal = source_mesh.normals.size() == source_mesh.vertices.size()
				? source_mesh.normals[index]
				: glm::vec3(0.0f, 0.0f, 1.0f);
		}
		normal = glm::normalize(normal);
		const glm::vec3 offset_position =
			source_mesh.vertices[index] + normal * outward_distance;
		if (!finite(offset_position)) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::NonFiniteGeometry,
				"ShellOffset outward surface contains a non-finite vertex.");
		}
		mesh->vertices.push_back(offset_position);
		mesh->normals.push_back(normal);
		mesh->texcoords.push_back(
			source_mesh.texcoords.size() == source_mesh.vertices.size()
				? source_mesh.texcoords[index]
				: glm::vec3(0.0f));
	}
	for (std::size_t index = 0; index < source_mesh.vertices.size(); ++index) {
		glm::vec3 normal = normal_sums[quantize(source_mesh.vertices[index])];
		if (glm::dot(normal, normal) <= 1.0e-16f) {
			normal = source_mesh.normals.size() == source_mesh.vertices.size()
				? source_mesh.normals[index]
				: glm::vec3(0.0f, 0.0f, 1.0f);
		}
		normal = glm::normalize(normal);
		const glm::vec3 offset_position =
			source_mesh.vertices[index] - normal * inward_distance;
		if (!finite(offset_position)) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::NonFiniteGeometry,
				"ShellOffset inward surface contains a non-finite vertex.");
		}
		mesh->vertices.push_back(offset_position);
		mesh->normals.push_back(-normal);
		mesh->texcoords.push_back(
			source_mesh.texcoords.size() == source_mesh.vertices.size()
				? source_mesh.texcoords[index]
				: glm::vec3(0.0f));
	}

	const int inner_offset = static_cast<int>(source_mesh.vertices.size());
	for (std::size_t face_index = 0; face_index < source_mesh.faces.size(); ++face_index) {
		const glm::ivec3 &face = source_mesh.faces[face_index];
		const glm::vec3 source_cross = glm::cross(
			source_mesh.vertices[static_cast<std::size_t>(face.y)] -
				source_mesh.vertices[static_cast<std::size_t>(face.x)],
			source_mesh.vertices[static_cast<std::size_t>(face.z)] -
				source_mesh.vertices[static_cast<std::size_t>(face.x)]);
		const glm::vec3 outer_cross = glm::cross(
			mesh->vertices[static_cast<std::size_t>(face.y)] -
				mesh->vertices[static_cast<std::size_t>(face.x)],
			mesh->vertices[static_cast<std::size_t>(face.z)] -
				mesh->vertices[static_cast<std::size_t>(face.x)]);
		const glm::vec3 inner_cross = glm::cross(
			mesh->vertices[static_cast<std::size_t>(face.y + inner_offset)] -
				mesh->vertices[static_cast<std::size_t>(face.x + inner_offset)],
			mesh->vertices[static_cast<std::size_t>(face.z + inner_offset)] -
				mesh->vertices[static_cast<std::size_t>(face.x + inner_offset)]);
		if (glm::dot(outer_cross, outer_cross) <= 1.0e-16f ||
		    glm::dot(inner_cross, inner_cross) <= 1.0e-16f ||
		    glm::dot(source_cross, outer_cross) <= 0.0f ||
		    glm::dot(source_cross, inner_cross) <= 0.0f) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::SelfIntersection,
				"ShellOffset thickness collapses or reverses at least one source face.");
		}
		mesh->faces.push_back(face);
		tags.push_back(source.faceSurfaceTags()[face_index]);
		mesh->faces.emplace_back(
			face.x + inner_offset,
			face.z + inner_offset,
			face.y + inner_offset);
		tags.emplace_back(MeshSurfaceRole::Inner, face_index);
	}
	for (const auto &edge : edges) {
		const BoundaryEdgeRecord &record = edge.second;
		if (record.incident_count != 1u) continue;
		const int outer_start = record.start_vertex;
		const int outer_end = record.end_vertex;
		const int inner_start = record.start_vertex + inner_offset;
		const int inner_end = record.end_vertex + inner_offset;
		mesh->faces.emplace_back(outer_start, inner_start, inner_end);
		mesh->faces.emplace_back(outer_start, inner_end, outer_end);
		tags.emplace_back(MeshSurfaceRole::Rim, 0u);
		tags.emplace_back(MeshSurfaceRole::Rim, 0u);
	}
	mesh->buildCollisionAccel();
	return GeometryBuildResult::createSuccess(
		GeneratedPrimitiveMesh(mesh, std::move(tags)));
}
