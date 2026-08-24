#include "chair/model/ChairObjectModel.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace {

bool finite_vector(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

glm::vec3 direction_from_rotation(const glm::mat4 &rotation, const glm::vec3 &direction)
{
	glm::vec3 world_direction = glm::normalize(
		glm::vec3(rotation * glm::vec4(direction, 0.0f)));
	for (int axis = 0; axis < 3; ++axis) {
		if (std::fabs(world_direction[axis]) < 1.0e-6f) {
			world_direction[axis] = 0.0f;
		} else if (std::fabs(world_direction[axis] - 1.0f) < 1.0e-6f) {
			world_direction[axis] = 1.0f;
		} else if (std::fabs(world_direction[axis] + 1.0f) < 1.0e-6f) {
			world_direction[axis] = -1.0f;
		}
	}
	return world_direction;
}

}

bool ChairContextApplicabilityRelationship::appliesTo(ChairUseContext context) const
{
	return std::find(contexts_.begin(), contexts_.end(), context) != contexts_.end();
}

bool ChairDesignAcceptanceRecord::passes(double threshold) const
{
	return front_iou_ > threshold && side_iou_ > threshold && top_iou_ > threshold;
}

glm::vec3 ChairOrientation::localUpDirection() const
{
	return glm::vec3(0.0f, 0.0f, 1.0f);
}

glm::vec3 ChairOrientation::localForwardDirection() const
{
	return glm::vec3(0.0f, -1.0f, 0.0f);
}

glm::mat4 ChairOrientation::localToWorldRotation() const
{
	glm::mat4 rotation(1.0f);
	rotation = glm::rotate(
		rotation, glm::radians(yaw_degrees_), glm::vec3(0.0f, 1.0f, 0.0f));
	rotation = glm::rotate(
		rotation, glm::radians(localToWorldPitchDegrees()), glm::vec3(1.0f, 0.0f, 0.0f));
	return rotation;
}

glm::vec3 ChairOrientation::worldUpDirection() const
{
	return direction_from_rotation(localToWorldRotation(), localUpDirection());
}

glm::vec3 ChairOrientation::worldForwardDirection() const
{
	return direction_from_rotation(localToWorldRotation(), localForwardDirection());
}

glm::vec3 ChairOrientation::worldRightDirection() const
{
	return direction_from_rotation(localToWorldRotation(), glm::vec3(1.0f, 0.0f, 0.0f));
}

bool ChairOrientation::isFinite() const
{
	return std::isfinite(yaw_degrees_);
}

ChairPlacement ChairPlacement::atWorldOrigin()
{
	return ChairPlacement(glm::vec3(0.0f), ChairOrientation(0.0f), 0.0f);
}

glm::mat4 ChairPlacement::worldTransform() const
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), ground_contact_position_);
	transform *= orientation_.localToWorldRotation();
	transform = glm::translate(
		transform, glm::vec3(0.0f, 0.0f, -local_ground_height_));
	return transform;
}

glm::vec3 ChairPlacement::worldPositionOfLocalPoint(const glm::vec3 &local_point) const
{
	return glm::vec3(worldTransform() * glm::vec4(local_point, 1.0f));
}

bool ChairPlacement::isValid() const
{
	return finite_vector(ground_contact_position_) && orientation_.isFinite() &&
	       std::isfinite(local_ground_height_);
}

ChairObjectModelBuildResult ChairObjectModelBuildResult::success(ChairObjectModel model)
{
	return ChairObjectModelBuildResult(std::move(model), {});
}

ChairObjectModelBuildResult ChairObjectModelBuildResult::failure(std::string diagnostic)
{
	return ChairObjectModelBuildResult(std::nullopt, std::move(diagnostic));
}

const char *chairUseContextName(ChairUseContext context)
{
	switch (context) {
	case ChairUseContext::Dining: return "Dining";
	case ChairUseContext::Kitchen: return "Kitchen";
	case ChairUseContext::Study: return "Study";
	case ChairUseContext::Living: return "Living";
	case ChairUseContext::Patio: return "Patio";
	}
	return "Unknown";
}

const char *chairSupportKindName(ChairSupportKind support_kind)
{
	switch (support_kind) {
	case ChairSupportKind::FourLeg: return "FourLeg";
	case ChairSupportKind::Cantilever: return "Cantilever";
	case ChairSupportKind::Sled: return "Sled";
	case ChairSupportKind::Pedestal: return "Pedestal";
	case ChairSupportKind::SwivelCaster: return "SwivelCaster";
	}
	return "Unknown";
}
