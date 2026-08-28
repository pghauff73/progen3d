#include "spatial/service/SpatialDirectionResolutionService.h"

#include "spatial/service/SpatialInterfaceQueryService.h"

#include <cmath>

namespace {

constexpr float kDirectionTolerance = 1.0e-7f;

glm::vec3 object_direction(const glm::mat4 &world_transform, const glm::vec3 &vector)
{
	return glm::mat3(world_transform) * vector;
}

glm::vec3 interface_direction(const SpatialInterfaceWorldFrame &frame,
	                          const glm::vec3 &vector)
{
	return frame.tangent() * vector.x +
	       frame.bitangent() * vector.y +
	       frame.normal() * vector.z;
}

} // namespace

std::optional<glm::vec3> SpatialDirectionResolutionService::resolve(
	const SpatialDirection &direction,
	const SpatialBuildingObject &moving_object,
	const SpatialInterface &moving_interface,
	const SpatialBuildingObject &target_object,
	const SpatialInterface &target_interface,
	const SpatialBuildingModel &model,
	std::string *diagnostic) const
{
	glm::vec3 world_direction(0.0f);
	const SpatialInterfaceQueryService interface_query;
	switch (direction.frame()) {
	case SpatialDirectionFrame::World:
		world_direction = direction.vector();
		break;
	case SpatialDirectionFrame::Parent: {
		const SpatialObjectId *parent_id =
			model.containmentTree().parentOf(moving_object.identity().objectId());
		if (parent_id == nullptr) {
			world_direction = direction.vector();
			break;
		}
		const SpatialBuildingObject *parent = model.objects().find(*parent_id);
		if (parent == nullptr) {
			if (diagnostic != nullptr) {
				*diagnostic = "Moving-object parent frame is undefined.";
			}
			return std::nullopt;
		}
		world_direction = object_direction(
			parent->frameState().resolvedWorldTransform(), direction.vector());
		break;
	}
	case SpatialDirectionFrame::MovingObject:
		world_direction = object_direction(
			moving_object.frameState().resolvedWorldTransform(), direction.vector());
		break;
	case SpatialDirectionFrame::MovingInterface:
		world_direction = interface_direction(
			interface_query.resolveWorldFrame(
				moving_interface,
				moving_object.frameState().resolvedWorldTransform()),
			direction.vector());
		break;
	case SpatialDirectionFrame::TargetObject:
		world_direction = object_direction(
			target_object.frameState().resolvedWorldTransform(), direction.vector());
		break;
	case SpatialDirectionFrame::TargetInterface:
		world_direction = interface_direction(
			interface_query.resolveWorldFrame(
				target_interface,
				target_object.frameState().resolvedWorldTransform()),
			direction.vector());
		break;
	}
	if (!std::isfinite(world_direction.x) || !std::isfinite(world_direction.y) ||
	    !std::isfinite(world_direction.z) ||
	    glm::length(world_direction) <= kDirectionTolerance) {
		if (diagnostic != nullptr) {
			*diagnostic = "Spatial positioning direction resolves to a non-finite or zero vector.";
		}
		return std::nullopt;
	}
	return glm::normalize(world_direction);
}
