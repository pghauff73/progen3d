#pragma once

#include "geometry/model/AxialTransitionKind.h"

#include <glm/glm.hpp>

#include <cstddef>

class AxialProfileLevel
{
public:
	AxialProfileLevel(float axial_position,
	                  std::size_t profile_index,
	                  glm::vec2 center,
	                  glm::vec2 scale,
	                  float rotation_degrees,
	                  AxialTransitionKind transition)
		: axial_position_(axial_position),
		  profile_index_(profile_index),
		  center_(center),
		  scale_(scale),
		  rotation_degrees_(rotation_degrees),
		  transition_(transition)
	{
	}

	float axialPosition() const
	{
		return axial_position_;
	}

	std::size_t profileIndex() const
	{
		return profile_index_;
	}

	const glm::vec2 &center() const
	{
		return center_;
	}

	const glm::vec2 &scale() const
	{
		return scale_;
	}

	float rotationDegrees() const
	{
		return rotation_degrees_;
	}

	AxialTransitionKind transition() const
	{
		return transition_;
	}

private:
	float axial_position_ = 0.0f;
	std::size_t profile_index_ = 0;
	glm::vec2 center_{0.0f};
	glm::vec2 scale_{1.0f};
	float rotation_degrees_ = 0.0f;
	AxialTransitionKind transition_ = AxialTransitionKind::Initial;
};
