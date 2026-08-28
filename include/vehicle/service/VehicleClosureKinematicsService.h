#pragma once

#include "vehicle/model/VehicleClosureAssembly.h"

#include <glm/glm.hpp>

#include <cstddef>

class VehicleClosureKinematicsService
{
public:
	glm::mat4 calculateHingePairTransform(
		const HingePair &hinge_pair,
		float normalized_state) const;

	glm::mat4 calculateFourBarTransform(
		const FourBarJoint &four_bar_joint,
		float normalized_state) const;

	glm::mat4 calculateClosureTransform(
		const ClosureKinematicRelationship &relationship,
		float normalized_state) const;

	glm::mat4 calculateDropGlassLocalTransform(
		const GuideRailJoint &guide_rail_joint,
		float normalized_state) const;

	glm::mat4 calculateDropGlassWorldTransform(
		const glm::mat4 &door_world_transform,
		const GuideRailJoint &guide_rail_joint,
		float normalized_state) const;

	SweptVolume calculateSweptVolume(
		const AxisAlignedBounds &source_bounds,
		const ClosureKinematicRelationship &relationship,
		std::size_t sample_count) const;
};
