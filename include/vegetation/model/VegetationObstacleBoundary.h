#pragma once

#include "geometry/model/AxisAlignedBounds.h"

#include <string>
#include <utility>

class VegetationObstacleBoundary
{
public:
	VegetationObstacleBoundary(
		std::string identifier,
		AxisAlignedBounds bounds,
		float clearance)
		: identifier_(std::move(identifier)),
		  bounds_(bounds),
		  clearance_(clearance)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const AxisAlignedBounds &bounds() const { return bounds_; }
	float clearance() const { return clearance_; }

private:
	std::string identifier_;
	AxisAlignedBounds bounds_;
	float clearance_ = 0.0f;
};
