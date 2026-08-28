#pragma once

#include "geometry/model/Profile2D.h"

#include <glm/glm.hpp>

#include <utility>

class LoftSectionSpecification
{
public:
	LoftSectionSpecification(
		float axial_position,
		Profile2D profile,
		glm::vec2 center = glm::vec2(0.0f),
		glm::vec2 scale = glm::vec2(1.0f),
		float rotation_degrees = 0.0f)
		: axial_position_(axial_position),
		  profile_(std::move(profile)),
		  center_(center),
		  scale_(scale),
		  rotation_degrees_(rotation_degrees)
	{
	}

	float axialPosition() const { return axial_position_; }
	const Profile2D &profile() const { return profile_; }
	const glm::vec2 &center() const { return center_; }
	const glm::vec2 &scale() const { return scale_; }
	float rotationDegrees() const { return rotation_degrees_; }

private:
	float axial_position_ = 0.0f;
	Profile2D profile_{{{}, ProfileWindingCorrection::None}, {}};
	glm::vec2 center_{0.0f};
	glm::vec2 scale_{1.0f};
	float rotation_degrees_ = 0.0f;
};
