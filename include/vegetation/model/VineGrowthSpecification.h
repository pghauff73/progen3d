#pragma once

#include "vegetation/model/SurfaceAttachmentMode.h"
#include "vegetation/model/VegetationCollisionBehavior.h"
#include "vegetation/model/VineGrowthMode.h"

#include <glm/glm.hpp>

#include <cstddef>

class VineGrowthSpecification
{
public:
	VineGrowthSpecification(
		VineGrowthMode growth_mode,
		VegetationCollisionBehavior collision_behavior,
		SurfaceAttachmentMode attachment_mode,
		float step_length,
		std::size_t maximum_segments,
		float maximum_seek_distance,
		float attachment_distance,
		float attachment_tolerance,
		float radius_decay,
		float minimum_radius,
		glm::vec3 preferred_direction,
		float radius_conservation_exponent = 2.0f)
		: growth_mode_(growth_mode),
		  collision_behavior_(collision_behavior),
		  attachment_mode_(attachment_mode),
		  step_length_(step_length),
		  maximum_segments_(maximum_segments),
		  maximum_seek_distance_(maximum_seek_distance),
		  attachment_distance_(attachment_distance),
		  attachment_tolerance_(attachment_tolerance),
		  radius_decay_(radius_decay),
		  minimum_radius_(minimum_radius),
		  preferred_direction_(preferred_direction),
		  radius_conservation_exponent_(radius_conservation_exponent)
	{
	}

	VineGrowthMode growthMode() const { return growth_mode_; }
	VegetationCollisionBehavior collisionBehavior() const
	{
		return collision_behavior_;
	}
	SurfaceAttachmentMode attachmentMode() const { return attachment_mode_; }
	float stepLength() const { return step_length_; }
	std::size_t maximumSegments() const { return maximum_segments_; }
	float maximumSeekDistance() const { return maximum_seek_distance_; }
	float attachmentDistance() const { return attachment_distance_; }
	float attachmentTolerance() const { return attachment_tolerance_; }
	float radiusDecay() const { return radius_decay_; }
	float minimumRadius() const { return minimum_radius_; }
	const glm::vec3 &preferredDirection() const { return preferred_direction_; }
	float radiusConservationExponent() const
	{
		return radius_conservation_exponent_;
	}

private:
	VineGrowthMode growth_mode_ = VineGrowthMode::FreeClimbing;
	VegetationCollisionBehavior collision_behavior_ =
		VegetationCollisionBehavior::Avoid;
	SurfaceAttachmentMode attachment_mode_ = SurfaceAttachmentMode::Offset;
	float step_length_ = 0.0f;
	std::size_t maximum_segments_ = 0;
	float maximum_seek_distance_ = 0.0f;
	float attachment_distance_ = 0.0f;
	float attachment_tolerance_ = 0.001f;
	float radius_decay_ = 0.0f;
	float minimum_radius_ = 0.0f;
	glm::vec3 preferred_direction_{0.0f, 1.0f, 0.0f};
	float radius_conservation_exponent_ = 2.0f;
};
