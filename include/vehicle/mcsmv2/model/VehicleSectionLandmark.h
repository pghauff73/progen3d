#pragma once

#include "vehicle/mcsmv2/model/VehicleSectionLandmarkRole.h"

class VehicleSectionLandmark
{
public:
	VehicleSectionLandmark(
		VehicleSectionLandmarkRole role,
		double half_width,
		double height)
		: role_(role),
		  half_width_(half_width),
		  height_(height)
	{
	}

	VehicleSectionLandmarkRole role() const { return role_; }
	double halfWidth() const { return half_width_; }
	double height() const { return height_; }

private:
	VehicleSectionLandmarkRole role_ = VehicleSectionLandmarkRole::Underbody;
	double half_width_ = 0.0;
	double height_ = 0.0;
};
