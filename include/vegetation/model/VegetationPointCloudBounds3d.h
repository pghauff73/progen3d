#pragma once

#include "vegetation/model/VegetationMeasuredPoint3d.h"

#include <cmath>

class VegetationPointCloudBounds3d
{
public:
	VegetationPointCloudBounds3d(
		VegetationMeasuredPoint3d minimum,
		VegetationMeasuredPoint3d maximum)
		: minimum_(minimum), maximum_(maximum)
	{
	}

	const VegetationMeasuredPoint3d &minimum() const { return minimum_; }
	const VegetationMeasuredPoint3d &maximum() const { return maximum_; }
	double extentX() const { return maximum_.x() - minimum_.x(); }
	double extentY() const { return maximum_.y() - minimum_.y(); }
	double extentZ() const { return maximum_.z() - minimum_.z(); }
	bool valid() const
	{
		return std::isfinite(minimum_.x()) && std::isfinite(minimum_.y()) &&
		       std::isfinite(minimum_.z()) && std::isfinite(maximum_.x()) &&
		       std::isfinite(maximum_.y()) && std::isfinite(maximum_.z()) &&
		       minimum_.x() <= maximum_.x() &&
		       minimum_.y() <= maximum_.y() &&
		       minimum_.z() <= maximum_.z();
	}

private:
	VegetationMeasuredPoint3d minimum_;
	VegetationMeasuredPoint3d maximum_;
};
