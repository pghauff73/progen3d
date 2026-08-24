#pragma once

#include "geometry/model/ProfileWindingCorrection.h"

#include <glm/glm.hpp>

#include <utility>
#include <vector>

class ProfileLoop2D
{
public:
	ProfileLoop2D(std::vector<glm::vec2> points,
	              ProfileWindingCorrection winding_correction)
		: points_(std::move(points)),
		  winding_correction_(winding_correction)
	{
	}

	const std::vector<glm::vec2> &points() const { return points_; }
	ProfileWindingCorrection windingCorrection() const { return winding_correction_; }

private:
	std::vector<glm::vec2> points_;
	ProfileWindingCorrection winding_correction_ = ProfileWindingCorrection::None;
};
