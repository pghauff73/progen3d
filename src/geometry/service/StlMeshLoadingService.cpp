#include "geometry/service/StlMeshLoadingService.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace {

struct QuantizedVertex
{
	std::int64_t x = 0;
	std::int64_t y = 0;
	std::int64_t z = 0;

	bool operator<(const QuantizedVertex &other) const
	{
		return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
	}
};

class WeldedTriangleMeshBuilder
{
public:
	explicit WeldedTriangleMeshBuilder(double quantization_metres)
		: quantization_metres_(quantization_metres)
	{
	}

	void addTriangle(
		const glm::vec3 &first,
		const glm::vec3 &second,
		const glm::vec3 &third)
	{
		mesh_->faces.emplace_back(
			vertexIndex(first), vertexIndex(second), vertexIndex(third));
	}

	std::shared_ptr<Mesh> finish()
	{
		mesh_->calc_normals();
		mesh_->buildCollisionAccel();
		return mesh_;
	}

private:
	int vertexIndex(const glm::vec3 &vertex)
	{
		const QuantizedVertex key{
			static_cast<std::int64_t>(std::llround(vertex.x / quantization_metres_)),
			static_cast<std::int64_t>(std::llround(vertex.y / quantization_metres_)),
			static_cast<std::int64_t>(std::llround(vertex.z / quantization_metres_))};
		const auto existing = vertex_indices_.find(key);
		if (existing != vertex_indices_.end()) return existing->second;
		const int index = static_cast<int>(mesh_->vertices.size());
		mesh_->vertices.emplace_back(
			static_cast<float>(key.x * quantization_metres_),
			static_cast<float>(key.y * quantization_metres_),
			static_cast<float>(key.z * quantization_metres_));
		vertex_indices_.emplace(key, index);
		return index;
	}

	double quantization_metres_ = 1.0e-6;
	std::shared_ptr<Mesh> mesh_ = std::make_shared<Mesh>();
	std::map<QuantizedVertex, int> vertex_indices_;
};

bool loadBinaryStl(
	std::ifstream &input,
	std::uint32_t triangle_count,
	WeldedTriangleMeshBuilder *builder)
{
	for (std::uint32_t triangle_index = 0u;
	     triangle_index < triangle_count;
	     ++triangle_index) {
		float normal[3];
		float vertices[9];
		std::uint16_t attribute_byte_count = 0u;
		if (!input.read(reinterpret_cast<char *>(normal), sizeof(normal)) ||
		    !input.read(reinterpret_cast<char *>(vertices), sizeof(vertices)) ||
		    !input.read(
			    reinterpret_cast<char *>(&attribute_byte_count),
			    sizeof(attribute_byte_count))) {
			return false;
		}
		builder->addTriangle(
			{vertices[0], vertices[1], vertices[2]},
			{vertices[3], vertices[4], vertices[5]},
			{vertices[6], vertices[7], vertices[8]});
	}
	return true;
}

bool loadAsciiStl(std::ifstream &input, WeldedTriangleMeshBuilder *builder)
{
	std::string token;
	std::vector<glm::vec3> triangle_vertices;
	triangle_vertices.reserve(3u);
	while (input >> token) {
		if (token != "vertex") continue;
		glm::vec3 vertex(0.0f);
		if (!(input >> vertex.x >> vertex.y >> vertex.z)) return false;
		triangle_vertices.push_back(vertex);
		if (triangle_vertices.size() == 3u) {
			builder->addTriangle(
				triangle_vertices[0], triangle_vertices[1], triangle_vertices[2]);
			triangle_vertices.clear();
		}
	}
	return triangle_vertices.empty();
}

} // namespace

GeneratedPrimitiveMesh StlMeshLoadingService::loadWeldedMesh(
	const std::filesystem::path &stl_path,
	double weld_quantization_metres) const
{
	if (!std::isfinite(weld_quantization_metres) || weld_quantization_metres <= 0.0) {
		throw std::invalid_argument("STL weld quantization must be finite and positive.");
	}
	std::error_code error;
	const std::uintmax_t file_size = std::filesystem::file_size(stl_path, error);
	if (error || file_size < 15u) {
		throw std::invalid_argument("STL file is missing or too small: " + stl_path.string());
	}
	std::ifstream input(stl_path, std::ios::binary);
	if (!input.is_open()) {
		throw std::invalid_argument("STL file could not be opened: " + stl_path.string());
	}
	WeldedTriangleMeshBuilder builder(weld_quantization_metres);
	char header[80] = {};
	std::uint32_t triangle_count = 0u;
	bool loaded = false;
	if (file_size >= 84u && input.read(header, sizeof(header)) &&
	    input.read(reinterpret_cast<char *>(&triangle_count), sizeof(triangle_count))) {
		const std::uintmax_t expected_size =
			84u + static_cast<std::uintmax_t>(triangle_count) * 50u;
		if (expected_size == file_size) {
			loaded = loadBinaryStl(input, triangle_count, &builder);
		}
	}
	if (!loaded) {
		input.clear();
		input.seekg(0, std::ios::beg);
		loaded = loadAsciiStl(input, &builder);
	}
	if (!loaded) {
		throw std::invalid_argument("STL file could not be parsed: " + stl_path.string());
	}
	std::shared_ptr<Mesh> mesh = builder.finish();
	if (mesh->faces.empty()) {
		throw std::invalid_argument("STL file contains no triangles: " + stl_path.string());
	}
	return GeneratedPrimitiveMesh(
		mesh,
		std::vector<MeshSurfaceTag>(
			mesh->faces.size(), MeshSurfaceTag(MeshSurfaceRole::Outer)));
}
