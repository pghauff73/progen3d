#include "geometry/service/GeneratedMeshComposer.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <memory>

namespace {

bool finite(const glm::mat4 &matrix)
{
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			if (!std::isfinite(matrix[column][row])) return false;
		}
	}
	return true;
}

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

}

GeometryBuildResult GeneratedMeshComposer::compose(
	const std::vector<GeneratedMeshPlacement> &placements) const
{
	std::size_t vertex_count = 0;
	std::size_t triangle_count = 0;
	for (std::size_t placement_index = 0;
	     placement_index < placements.size();
	     ++placement_index) {
		const GeneratedMeshPlacement &placement = placements[placement_index];
		if (!finite(placement.localTransform())) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::NonFiniteGeometry,
				"Generated mesh placement '" + placement.purpose() +
				"' has a non-finite transform.");
		}
		const GeneratedPrimitiveMesh &generated = placement.generatedMesh();
		if (!generated.mesh() ||
		    generated.faceSurfaceTags().size() != generated.mesh()->faces.size()) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::UnsupportedTopology,
				"Generated mesh placement '" + placement.purpose() +
				"' has invalid mesh-to-surface-tag correspondence.");
		}
		vertex_count += generated.mesh()->vertices.size();
		triangle_count += generated.mesh()->faces.size();
	}
	if (vertex_count > complexity_limits_.maximumGeneratedVertices() ||
	    triangle_count > complexity_limits_.maximumGeneratedTriangles()) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::TriangleLimitExceeded,
			"Generated mesh composition exceeds the configured mesh limits.");
	}

	auto combined_mesh = std::make_shared<Mesh>();
	combined_mesh->vertices.reserve(vertex_count);
	combined_mesh->normals.reserve(vertex_count);
	combined_mesh->texcoords.reserve(vertex_count);
	combined_mesh->faces.reserve(triangle_count);
	std::vector<MeshSurfaceTag> combined_tags;
	combined_tags.reserve(triangle_count);

	for (std::size_t placement_index = 0;
	     placement_index < placements.size();
	     ++placement_index) {
		const GeneratedMeshPlacement &placement = placements[placement_index];
		const Mesh &source = *placement.generatedMesh().mesh();
		const int vertex_offset = static_cast<int>(combined_mesh->vertices.size());
		const glm::mat3 normal_transform =
			glm::transpose(glm::inverse(glm::mat3(placement.localTransform())));
		for (std::size_t vertex_index = 0;
		     vertex_index < source.vertices.size();
		     ++vertex_index) {
			const glm::vec3 transformed_position = glm::vec3(
				placement.localTransform() * glm::vec4(source.vertices[vertex_index], 1.0f));
			if (!finite(transformed_position)) {
				return GeometryBuildResult::createFailure(
					GeometryBuildStatus::NonFiniteGeometry,
					"Generated mesh composition produced a non-finite vertex.");
			}
			combined_mesh->vertices.push_back(transformed_position);
			if (source.normals.size() == source.vertices.size()) {
				combined_mesh->normals.push_back(glm::normalize(
					normal_transform * source.normals[vertex_index]));
			}
			else {
				combined_mesh->normals.emplace_back(0.0f);
			}
			if (source.texcoords.size() == source.vertices.size()) {
				combined_mesh->texcoords.push_back(source.texcoords[vertex_index]);
			}
			else {
				combined_mesh->texcoords.emplace_back(0.0f);
			}
		}
		for (const glm::ivec3 &face : source.faces) {
			combined_mesh->faces.emplace_back(face + glm::ivec3(vertex_offset));
		}
		for (const MeshSurfaceTag &source_tag :
		     placement.generatedMesh().faceSurfaceTags()) {
			combined_tags.emplace_back(
				source_tag.role(),
				source_tag.boundaryIndex(),
				placement_index);
		}
	}
	if (combined_mesh->faces.empty()) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			"Generated mesh composition requires at least one mesh part.");
	}
	combined_mesh->buildCollisionAccel();
	return GeometryBuildResult::createSuccess(
		GeneratedPrimitiveMesh(combined_mesh, std::move(combined_tags)));
}
