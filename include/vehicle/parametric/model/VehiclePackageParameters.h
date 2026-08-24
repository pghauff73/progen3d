#pragma once

#include <string>
#include <utility>

class VehiclePackageParameters
{
public:
	VehiclePackageParameters(
		std::string identifier,
		double length,
		double width,
		double height,
		double wheelbase,
		double ground_clearance,
		double front_overhang,
		double rear_overhang)
		: identifier_(std::move(identifier)),
		  length_(length),
		  width_(width),
		  height_(height),
		  wheelbase_(wheelbase),
		  ground_clearance_(ground_clearance),
		  front_overhang_(front_overhang),
		  rear_overhang_(rear_overhang)
	{
	}

	const std::string &identifier() const { return identifier_; }
	double length() const { return length_; }
	double width() const { return width_; }
	double height() const { return height_; }
	double wheelbase() const { return wheelbase_; }
	double groundClearance() const { return ground_clearance_; }
	double frontOverhang() const { return front_overhang_; }
	double rearOverhang() const { return rear_overhang_; }

	double sourceFrontBumperStation() const
	{
		return wheelbase_ * 0.5 + front_overhang_;
	}
	double sourceRearBumperStation() const
	{
		return -(wheelbase_ * 0.5 + rear_overhang_);
	}
	double sourcePackageCenterStation() const
	{
		return (sourceFrontBumperStation() + sourceRearBumperStation()) * 0.5;
	}
	double frontAxleStation() const { return length_ * 0.5 - front_overhang_; }
	double rearAxleStation() const { return -length_ * 0.5 + rear_overhang_; }

private:
	std::string identifier_;
	double length_ = 0.0;
	double width_ = 0.0;
	double height_ = 0.0;
	double wheelbase_ = 0.0;
	double ground_clearance_ = 0.0;
	double front_overhang_ = 0.0;
	double rear_overhang_ = 0.0;
};
