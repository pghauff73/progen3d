#pragma once

#include <array>

class VehicleExteriorFeatureDefinition
{
public:
	VehicleExteriorFeatureDefinition(
		std::array<int, 3> body_rgb,
		int exhaust_outlet_count)
		: body_rgb_(body_rgb), exhaust_outlet_count_(exhaust_outlet_count)
	{
	}

	const std::array<int, 3> &bodyRgb() const { return body_rgb_; }
	int exhaustOutletCount() const { return exhaust_outlet_count_; }

private:
	std::array<int, 3> body_rgb_{{0, 0, 0}};
	int exhaust_outlet_count_ = 0;
};
