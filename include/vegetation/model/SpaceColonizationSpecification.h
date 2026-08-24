#pragma once

#include <glm/glm.hpp>

#include <cstddef>

class SpaceColonizationSpecification
{
public:
	SpaceColonizationSpecification(
		float influence_radius,
		float kill_radius,
		float step_length,
		std::size_t maximum_iterations,
		std::size_t attraction_point_count,
		float radius_decay,
		float minimum_radius,
		float radius_conservation_exponent,
		glm::vec3 tropism_direction = glm::vec3(0.0f),
		float tropism_weight = 0.0f)
		: influence_radius_(influence_radius),
		  kill_radius_(kill_radius),
		  step_length_(step_length),
		  maximum_iterations_(maximum_iterations),
		  attraction_point_count_(attraction_point_count),
		  radius_decay_(radius_decay),
		  minimum_radius_(minimum_radius),
		  radius_conservation_exponent_(radius_conservation_exponent),
		  tropism_direction_(tropism_direction),
		  tropism_weight_(tropism_weight)
	{
	}

	float influenceRadius() const { return influence_radius_; }
	float killRadius() const { return kill_radius_; }
	float stepLength() const { return step_length_; }
	std::size_t maximumIterations() const { return maximum_iterations_; }
	std::size_t attractionPointCount() const
	{
		return attraction_point_count_;
	}
	float radiusDecay() const { return radius_decay_; }
	float minimumRadius() const { return minimum_radius_; }
	float radiusConservationExponent() const
	{
		return radius_conservation_exponent_;
	}
	const glm::vec3 &tropismDirection() const { return tropism_direction_; }
	float tropismWeight() const { return tropism_weight_; }

private:
	float influence_radius_ = 0.0f;
	float kill_radius_ = 0.0f;
	float step_length_ = 0.0f;
	std::size_t maximum_iterations_ = 0;
	std::size_t attraction_point_count_ = 0;
	float radius_decay_ = 0.0f;
	float minimum_radius_ = 0.0f;
	float radius_conservation_exponent_ = 2.0f;
	glm::vec3 tropism_direction_{0.0f};
	float tropism_weight_ = 0.0f;
};
