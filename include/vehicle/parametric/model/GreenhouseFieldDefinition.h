#pragma once

#include "vehicle/parametric/model/StationFunction.h"

class GreenhouseFieldDefinition
{
public:
	GreenhouseFieldDefinition(
		double longitudinal_exponent,
		double lateral_exponent,
		double vertical_exponent,
		double width_scale,
		double roof_crown_longitudinal_exponent,
		double roof_crown_vertical_exponent,
		StationFunction width_ratio_by_station,
		StationFunction centre_height_by_station,
		StationFunction half_height_by_station)
		: longitudinal_exponent_(longitudinal_exponent),
		  lateral_exponent_(lateral_exponent),
		  vertical_exponent_(vertical_exponent),
		  width_scale_(width_scale),
		  roof_crown_longitudinal_exponent_(roof_crown_longitudinal_exponent),
		  roof_crown_vertical_exponent_(roof_crown_vertical_exponent),
		  width_ratio_by_station_(std::move(width_ratio_by_station)),
		  centre_height_by_station_(std::move(centre_height_by_station)),
		  half_height_by_station_(std::move(half_height_by_station))
	{
	}

	double longitudinalExponent() const { return longitudinal_exponent_; }
	double lateralExponent() const { return lateral_exponent_; }
	double verticalExponent() const { return vertical_exponent_; }
	double widthScale() const { return width_scale_; }
	double roofCrownLongitudinalExponent() const
	{
		return roof_crown_longitudinal_exponent_;
	}
	double roofCrownVerticalExponent() const
	{
		return roof_crown_vertical_exponent_;
	}
	const StationFunction &widthRatioByStation() const
	{
		return width_ratio_by_station_;
	}
	const StationFunction &centreHeightByStation() const
	{
		return centre_height_by_station_;
	}
	const StationFunction &halfHeightByStation() const
	{
		return half_height_by_station_;
	}

private:
	double longitudinal_exponent_ = 0.0;
	double lateral_exponent_ = 0.0;
	double vertical_exponent_ = 0.0;
	double width_scale_ = 1.0;
	double roof_crown_longitudinal_exponent_ = 0.0;
	double roof_crown_vertical_exponent_ = 0.0;
	StationFunction width_ratio_by_station_{"", StationInterpolationKind::Pchip, {}};
	StationFunction centre_height_by_station_{"", StationInterpolationKind::Pchip, {}};
	StationFunction half_height_by_station_{"", StationInterpolationKind::Pchip, {}};
};
