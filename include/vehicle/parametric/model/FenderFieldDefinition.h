#pragma once

class FenderFieldDefinition
{
public:
	FenderFieldDefinition(
		double front_longitudinal_radius,
		double rear_longitudinal_radius,
		double front_vertical_radius,
		double rear_vertical_radius,
		double centre_height,
		double half_width,
		double longitudinal_exponent,
		double lateral_exponent,
		double vertical_exponent)
		: front_longitudinal_radius_(front_longitudinal_radius),
		  rear_longitudinal_radius_(rear_longitudinal_radius),
		  front_vertical_radius_(front_vertical_radius),
		  rear_vertical_radius_(rear_vertical_radius),
		  centre_height_(centre_height),
		  half_width_(half_width),
		  longitudinal_exponent_(longitudinal_exponent),
		  lateral_exponent_(lateral_exponent),
		  vertical_exponent_(vertical_exponent)
	{
	}

	double frontLongitudinalRadius() const { return front_longitudinal_radius_; }
	double rearLongitudinalRadius() const { return rear_longitudinal_radius_; }
	double frontVerticalRadius() const { return front_vertical_radius_; }
	double rearVerticalRadius() const { return rear_vertical_radius_; }
	double centreHeight() const { return centre_height_; }
	double halfWidth() const { return half_width_; }
	double longitudinalExponent() const { return longitudinal_exponent_; }
	double lateralExponent() const { return lateral_exponent_; }
	double verticalExponent() const { return vertical_exponent_; }

private:
	double front_longitudinal_radius_ = 0.0;
	double rear_longitudinal_radius_ = 0.0;
	double front_vertical_radius_ = 0.0;
	double rear_vertical_radius_ = 0.0;
	double centre_height_ = 0.0;
	double half_width_ = 0.0;
	double longitudinal_exponent_ = 0.0;
	double lateral_exponent_ = 0.0;
	double vertical_exponent_ = 0.0;
};
