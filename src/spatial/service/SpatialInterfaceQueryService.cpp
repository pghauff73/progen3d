#include "spatial/service/SpatialInterfaceQueryService.h"

#include "spatial/service/SpatialAabbQueryService.h"

#include <array>
#include <cmath>
#include <limits>

namespace {

constexpr float kDirectionTolerance = 1.0e-7f;

glm::vec3 normalized_or(const glm::vec3 &vector, const glm::vec3 &fallback)
{
	return glm::length(vector) > kDirectionTolerance ? glm::normalize(vector) : fallback;
}

AxisAlignedBounds bounds_from_points(const std::vector<glm::vec3> &points)
{
	if (points.empty()) return {};
	glm::vec3 minimum(std::numeric_limits<float>::max());
	glm::vec3 maximum(std::numeric_limits<float>::lowest());
	for (const glm::vec3 &point : points) {
		minimum = glm::min(minimum, point);
		maximum = glm::max(maximum, point);
	}
	AxisAlignedBounds bounds;
	bounds.min = minimum;
	bounds.max = maximum;
	bounds.center = (minimum + maximum) * 0.5f;
	bounds.half_extents = (maximum - minimum) * 0.5f;
	bounds.valid = true;
	return bounds;
}

AxisAlignedBounds boundary_face_bounds(const AxisAlignedBounds &object_bounds,
	                                   const std::string &face_name)
{
	if (!object_bounds.valid) return {};
	AxisAlignedBounds face = object_bounds;
	if (face_name == "top") face.min.y = face.max.y;
	else if (face_name == "bottom") face.max.y = face.min.y;
	else if (face_name == "left") face.max.x = face.min.x;
	else if (face_name == "right") face.min.x = face.max.x;
	else if (face_name == "front") face.min.z = face.max.z;
	else if (face_name == "back") face.max.z = face.min.z;
	else return {};
	face.center = (face.min + face.max) * 0.5f;
	face.half_extents = (face.max - face.min) * 0.5f;
	return face;
}

} // namespace

SpatialInterfaceWorldFrame SpatialInterfaceQueryService::resolveWorldFrame(
	const SpatialInterface &interface,
	const glm::mat4 &object_world_transform) const
{
	const glm::vec3 origin = glm::vec3(
		object_world_transform * glm::vec4(interface.localFrame().localOrigin(), 1.0f));
	const glm::mat3 linear(object_world_transform);
	const float determinant = glm::determinant(linear);
	const glm::mat3 normal_transform = std::fabs(determinant) > kDirectionTolerance
		? glm::transpose(glm::inverse(linear))
		: glm::mat3(1.0f);
	const glm::vec3 normal = normalized_or(
		normal_transform * interface.localFrame().localNormal(),
		glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 tangent = linear * interface.localFrame().localTangent();
	tangent -= normal * glm::dot(tangent, normal);
	tangent = normalized_or(tangent, glm::vec3(1.0f, 0.0f, 0.0f));
	if (std::fabs(glm::dot(tangent, normal)) > 0.999f) {
		tangent = normalized_or(glm::cross(normal, glm::vec3(0.0f, 0.0f, 1.0f)),
		                        glm::vec3(1.0f, 0.0f, 0.0f));
	}
	const glm::vec3 bitangent = normalized_or(
		glm::cross(normal, tangent), glm::vec3(0.0f, 0.0f, 1.0f));
	return SpatialInterfaceWorldFrame(origin, normal, tangent, bitangent);
}

SpatialInterfaceProjection SpatialInterfaceQueryService::project(
	const SpatialBuildingObject &object,
	const SpatialInterface &interface,
	const glm::mat4 &object_world_transform,
	const AxisAlignedBounds &object_world_bounds) const
{
	(void)object;
	const SpatialInterfaceWorldFrame world_frame =
		resolveWorldFrame(interface, object_world_transform);
	std::vector<glm::vec3> points;
	switch (interface.region().kind()) {
	case SpatialInterfaceRegionKind::Point:
		points.push_back(world_frame.origin());
		break;
	case SpatialInterfaceRegionKind::PlaneRectangle: {
		const glm::vec3 tangent_extent =
			world_frame.tangent() * (interface.region().primaryExtent() * 0.5f);
		const glm::vec3 bitangent_extent =
			world_frame.bitangent() * (interface.region().secondaryExtent() * 0.5f);
		points = {world_frame.origin() - tangent_extent - bitangent_extent,
		          world_frame.origin() + tangent_extent - bitangent_extent,
		          world_frame.origin() - tangent_extent + bitangent_extent,
		          world_frame.origin() + tangent_extent + bitangent_extent};
		break;
	}
	case SpatialInterfaceRegionKind::AxisSegment: {
		const glm::vec3 extent =
			world_frame.normal() * (interface.region().primaryExtent() * 0.5f);
		points = {world_frame.origin() - extent, world_frame.origin() + extent};
		break;
	}
	case SpatialInterfaceRegionKind::ObjectBoundaryFace:
		return SpatialInterfaceProjection(
			world_frame,
			boundary_face_bounds(
				object_world_bounds, interface.region().boundaryFaceName()));
	}
	return SpatialInterfaceProjection(world_frame, bounds_from_points(points));
}
