#include "geometry/service/AffineCubeFaceQuery.h"

#include "Context.h"

#include <algorithm>
#include <cmath>

bool hasAffineCubeFaces(const ScenePrimitiveInstance &primitive)
{
	if (primitive.type != "Cube" || !primitive.resolved_geometry ||
	    !primitive.resolved_geometry->usesCubeVertexTemplate() ||
	    primitive.primary_transform != primitive.secondary_transform) {
		return false;
	}
	const float determinant = glm::determinant(glm::mat3(primitive.primary_transform));
	return std::isfinite(determinant) && determinant != 0.0f &&
	       std::all_of(primitive.dual_scales.begin(), primitive.dual_scales.end(),
	                   [](const glm::vec3 &scale) { return scale == glm::vec3(1.0f); }) &&
	       std::all_of(primitive.dual_translations.begin(), primitive.dual_translations.end(),
	                   [](const glm::vec3 &offset) { return offset == glm::vec3(0.0f); });
}

std::optional<CubeFace> cubeFaceAtWorldPoint(const ScenePrimitiveInstance &primitive,
                                            const glm::vec3 &world_point)
{
	if (!hasAffineCubeFaces(primitive)) return std::nullopt;
	const glm::vec3 local_point =
		glm::vec3(glm::inverse(primitive.primary_transform) * glm::vec4(world_point, 1.0f));
	constexpr float face_tolerance = 0.0002f;
	std::optional<CubeFace> face;
	for (int axis = 0; axis < 3; ++axis) {
		const float coordinate = local_point[axis];
		if (!std::isfinite(coordinate) || std::fabs(coordinate) > 0.5f + face_tolerance) {
			return std::nullopt;
		}
		if (std::fabs(std::fabs(coordinate) - 0.5f) <= face_tolerance) {
			if (face.has_value()) return std::nullopt; // Edges do not identify one face.
			face = cube_faces[static_cast<std::size_t>(axis * 2 + (coordinate > 0.0f))];
		}
	}
	return face;
}

std::array<glm::vec3, 4> cubeFaceWorldCorners(const ScenePrimitiveInstance &primitive,
                                             CubeFace face)
{
	const int face_index = static_cast<int>(face);
	const int normal_axis = face_index / 2;
	const int axis_u = (normal_axis + 1) % 3;
	const int axis_v = (normal_axis + 2) % 3;
	constexpr std::array<glm::vec2, 4> corners{{
		{-0.5f, -0.5f}, {0.5f, -0.5f}, {0.5f, 0.5f}, {-0.5f, 0.5f}}};
	std::array<glm::vec3, 4> world_corners;
	for (std::size_t index = 0; index < corners.size(); ++index) {
		glm::vec3 local_point;
		local_point[normal_axis] = face_index % 2 == 0 ? -0.5f : 0.5f;
		local_point[axis_u] = corners[index].x;
		local_point[axis_v] = corners[index].y;
		world_corners[index] = glm::vec3(primitive.primary_transform * glm::vec4(local_point, 1.0f));
	}
	return world_corners;
}
