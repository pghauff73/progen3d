#include "geometry/service/MirroredShapeGeometryBuilder.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <memory>

namespace {

glm::vec3 reflect_position(glm::vec3 position, int axis, float offset)
{
	position[axis] = 2.0f * offset - position[axis];
	return position;
}

glm::vec3 reflect_direction(glm::vec3 direction, int axis)
{
	direction[axis] = -direction[axis];
	return direction;
}

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

}

GeometryBuildResult MirroredShapeGeometryBuilder::build(
	const GeneratedPrimitiveMesh &source,
	const MirrorShapeSpecification &specification) const
{
	if (!source.mesh() || source.mesh()->faces.empty() ||
	    source.faceSurfaceTags().size() != source.mesh()->faces.size()) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			"MirrorShape requires a non-empty source mesh with matching surface tags.");
	}
	if (specification.planeAxis() < 0 || specification.planeAxis() > 2 ||
	    !std::isfinite(specification.planeOffset())) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::InvalidSpecification,
			"MirrorShape requires axis x, y, or z and a finite plane offset.");
	}

	const Mesh &source_mesh = *source.mesh();
	const bool include_source =
		specification.mode() == MirrorShapeMode::SourceAndMirrored;
	const std::size_t copy_count = include_source ? 2u : 1u;
	if (source_mesh.vertices.size() * copy_count >
	        complexity_limits_.maximumGeneratedVertices() ||
	    source_mesh.faces.size() * copy_count >
	        complexity_limits_.maximumGeneratedTriangles()) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::TriangleLimitExceeded,
			"MirrorShape exceeds the configured generated mesh limits.");
	}

	auto mesh = std::make_shared<Mesh>();
	mesh->vertices.reserve(source_mesh.vertices.size() * copy_count);
	mesh->normals.reserve(source_mesh.vertices.size() * copy_count);
	mesh->texcoords.reserve(source_mesh.vertices.size() * copy_count);
	mesh->faces.reserve(source_mesh.faces.size() * copy_count);
	std::vector<MeshSurfaceTag> tags;
	tags.reserve(source.faceSurfaceTags().size() * copy_count);

	if (include_source) {
		mesh->vertices = source_mesh.vertices;
		mesh->normals = source_mesh.normals;
		mesh->texcoords = source_mesh.texcoords;
		mesh->faces = source_mesh.faces;
		tags = source.faceSurfaceTags();
	}

	const int vertex_offset = static_cast<int>(mesh->vertices.size());
	for (std::size_t index = 0; index < source_mesh.vertices.size(); ++index) {
		const glm::vec3 position = reflect_position(
			source_mesh.vertices[index], specification.planeAxis(),
			specification.planeOffset());
		if (!finite(position)) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::NonFiniteGeometry,
				"MirrorShape produced a non-finite vertex.");
		}
		mesh->vertices.push_back(position);
		if (source_mesh.normals.size() == source_mesh.vertices.size()) {
			mesh->normals.push_back(glm::normalize(reflect_direction(
				source_mesh.normals[index], specification.planeAxis())));
		}
		else {
			mesh->normals.emplace_back(0.0f);
		}
		mesh->texcoords.push_back(
			source_mesh.texcoords.size() == source_mesh.vertices.size()
				? source_mesh.texcoords[index]
				: glm::vec3(0.0f));
	}
	for (const glm::ivec3 &face : source_mesh.faces) {
		mesh->faces.emplace_back(
			face.x + vertex_offset,
			face.z + vertex_offset,
			face.y + vertex_offset);
	}
	tags.insert(
		tags.end(), source.faceSurfaceTags().begin(),
		source.faceSurfaceTags().end());
	mesh->buildCollisionAccel();
	return GeometryBuildResult::createSuccess(
		GeneratedPrimitiveMesh(mesh, std::move(tags)));
}
