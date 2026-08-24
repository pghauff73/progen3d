#include "vehicle/service/VehicleClosureKinematicsService.h"

#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <array>
#include <limits>

namespace {

float normalized_state(float state)
{
	return glm::clamp(state, 0.0f, 1.0f);
}

glm::mat4 rotation_about_pivot(glm::vec3 pivot, glm::vec3 axis, float degrees)
{
	glm::mat4 transform = glm::translate(glm::mat4(1.0f), pivot);
	transform = glm::rotate(transform, glm::radians(degrees), axis);
	return glm::translate(transform, -pivot);
}

std::array<glm::vec3, 8> bounds_corners(const AxisAlignedBounds &bounds)
{
	return {{
		{bounds.min.x, bounds.min.y, bounds.min.z},
		{bounds.max.x, bounds.min.y, bounds.min.z},
		{bounds.min.x, bounds.max.y, bounds.min.z},
		{bounds.max.x, bounds.max.y, bounds.min.z},
		{bounds.min.x, bounds.min.y, bounds.max.z},
		{bounds.max.x, bounds.min.y, bounds.max.z},
		{bounds.min.x, bounds.max.y, bounds.max.z},
		{bounds.max.x, bounds.max.y, bounds.max.z}}};
}

void include_transformed_bounds(
	const AxisAlignedBounds &source_bounds,
	const glm::mat4 &transform,
	glm::vec3 *minimum,
	glm::vec3 *maximum)
{
	for (const glm::vec3 &corner : bounds_corners(source_bounds)) {
		const glm::vec3 transformed = glm::vec3(transform * glm::vec4(corner, 1.0f));
		*minimum = glm::min(*minimum, transformed);
		*maximum = glm::max(*maximum, transformed);
	}
}

glm::quat rotation_between_directions(glm::vec3 source, glm::vec3 target)
{
	source = glm::normalize(source);
	target = glm::normalize(target);
	const float cosine = glm::dot(source, target);
	if (cosine > 1.0f - 1.0e-6f) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
	if (cosine < -1.0f + 1.0e-6f) {
		glm::vec3 axis = glm::cross(source, glm::vec3(1.0f, 0.0f, 0.0f));
		if (glm::length(axis) <= 1.0e-6f) {
			axis = glm::cross(source, glm::vec3(0.0f, 1.0f, 0.0f));
		}
		return glm::angleAxis(glm::pi<float>(), glm::normalize(axis));
	}
	return glm::normalize(glm::quat(
		1.0f + cosine, glm::cross(source, target)));
}

} // namespace

glm::mat4 VehicleClosureKinematicsService::calculateHingePairTransform(
	const HingePair &hinge_pair,
	float normalized_state_value) const
{
	const glm::vec3 axis = hinge_pair.axis();
	if (glm::length(axis) <= 1.0e-6f) return glm::mat4(1.0f);
	return rotation_about_pivot(
		hinge_pair.lowerHingePoint(), axis,
		normalized_state(normalized_state_value) *
			hinge_pair.maximumAngleDegrees());
}

glm::mat4 VehicleClosureKinematicsService::calculateFourBarTransform(
	const FourBarJoint &four_bar_joint,
	float normalized_state_value) const
{
	const float state = normalized_state(normalized_state_value);
	const glm::vec3 body_axis = four_bar_joint.bodyMountB() - four_bar_joint.bodyMountA();
	const float axis_length = glm::length(body_axis);
	if (axis_length <= 1.0e-6f) return glm::mat4(1.0f);
	const glm::vec3 pivot =
		(four_bar_joint.bodyMountA() + four_bar_joint.bodyMountB()) * 0.5f;
	glm::mat4 transform = glm::translate(
		glm::mat4(1.0f), four_bar_joint.openTranslation() * state);
	return transform * rotation_about_pivot(
		pivot, body_axis / axis_length,
		state * four_bar_joint.maximumAngleDegrees());
}

glm::mat4 VehicleClosureKinematicsService::calculateClosureTransform(
	const ClosureKinematicRelationship &relationship,
	float normalized_state_value) const
{
	if (std::holds_alternative<HingePair>(relationship)) {
		return calculateHingePairTransform(
			std::get<HingePair>(relationship), normalized_state_value);
	}
	return calculateFourBarTransform(
		std::get<FourBarJoint>(relationship), normalized_state_value);
}

glm::mat4 VehicleClosureKinematicsService::calculateDropGlassLocalTransform(
	const GuideRailJoint &guide_rail_joint,
	float normalized_state_value) const
{
	const float state = glm::mix(
		guide_rail_joint.lowerLimit(), guide_rail_joint.upperLimit(),
		normalized_state(normalized_state_value));
	const glm::vec3 target_front = guide_rail_joint.frontRail().evaluate(state);
	const glm::vec3 target_rear = guide_rail_joint.rearRail().evaluate(state);
	const glm::vec3 source_direction =
		guide_rail_joint.rearFollower() - guide_rail_joint.frontFollower();
	const glm::vec3 target_direction = target_rear - target_front;
	if (glm::length(source_direction) <= 1.0e-6f ||
	    glm::length(target_direction) <= 1.0e-6f) {
		return glm::mat4(1.0f);
	}
	const glm::quat orientation = rotation_between_directions(
		source_direction, target_direction);
	const glm::mat4 rotation = glm::mat4_cast(orientation);
	const glm::vec3 source_midpoint =
		(guide_rail_joint.frontFollower() + guide_rail_joint.rearFollower()) * 0.5f;
	const glm::vec3 target_midpoint = (target_front + target_rear) * 0.5f;
	const glm::vec3 rotated_source_midpoint =
		glm::vec3(rotation * glm::vec4(source_midpoint, 1.0f));
	return glm::translate(
		glm::mat4(1.0f), target_midpoint - rotated_source_midpoint) * rotation;
}

glm::mat4 VehicleClosureKinematicsService::calculateDropGlassWorldTransform(
	const glm::mat4 &door_world_transform,
	const GuideRailJoint &guide_rail_joint,
	float normalized_state_value) const
{
	return door_world_transform * calculateDropGlassLocalTransform(
		guide_rail_joint, normalized_state_value);
}

SweptVolume VehicleClosureKinematicsService::calculateSweptVolume(
	const AxisAlignedBounds &source_bounds,
	const ClosureKinematicRelationship &relationship,
	std::size_t sample_count) const
{
	if (!source_bounds.valid || sample_count < 2u) {
		return SweptVolume(source_bounds, AxisAlignedBounds{}, 0.0f, 1.0f, sample_count);
	}
	glm::vec3 minimum(std::numeric_limits<float>::max());
	glm::vec3 maximum(std::numeric_limits<float>::lowest());
	for (std::size_t sample_index = 0u; sample_index < sample_count; ++sample_index) {
		const float state = static_cast<float>(sample_index) /
			static_cast<float>(sample_count - 1u);
		include_transformed_bounds(
			source_bounds, calculateClosureTransform(relationship, state),
			&minimum, &maximum);
	}
	AxisAlignedBounds swept_bounds;
	swept_bounds.min = minimum;
	swept_bounds.max = maximum;
	swept_bounds.center = (minimum + maximum) * 0.5f;
	swept_bounds.half_extents = (maximum - minimum) * 0.5f;
	swept_bounds.valid = true;
	return SweptVolume(source_bounds, swept_bounds, 0.0f, 1.0f, sample_count);
}
