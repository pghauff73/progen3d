#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>

class VehiclePackageEnvelope
{
public:
	VehiclePackageEnvelope(
		std::string identifier = {},
		glm::vec3 minimum = glm::vec3(0.0f),
		glm::vec3 maximum = glm::vec3(0.0f))
		: identifier_(std::move(identifier)),
		  minimum_(minimum),
		  maximum_(maximum)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const glm::vec3 &minimum() const { return minimum_; }
	const glm::vec3 &maximum() const { return maximum_; }
	glm::vec3 center() const { return (minimum_ + maximum_) * 0.5f; }
	glm::vec3 dimensions() const { return maximum_ - minimum_; }
	bool isFiniteAndOrdered() const;

private:
	std::string identifier_;
	glm::vec3 minimum_{0.0f};
	glm::vec3 maximum_{0.0f};
};

class VehiclePackage
{
public:
	VehiclePackage(
		float overall_length,
		float overall_width,
		float overall_height,
		float wheelbase,
		float front_track,
		float rear_track,
		float ground_clearance,
		float front_overhang,
		float rear_overhang,
		float front_wheel_radius,
		float rear_wheel_radius,
		VehiclePackageEnvelope cabin = {},
		VehiclePackageEnvelope front_powertrain = {},
		VehiclePackageEnvelope rear_powertrain = {},
		VehiclePackageEnvelope battery = {},
		VehiclePackageEnvelope luggage = {})
		: overall_length_(overall_length),
		  overall_width_(overall_width),
		  overall_height_(overall_height),
		  wheelbase_(wheelbase),
		  front_track_(front_track),
		  rear_track_(rear_track),
		  ground_clearance_(ground_clearance),
		  front_overhang_(front_overhang),
		  rear_overhang_(rear_overhang),
		  front_wheel_radius_(front_wheel_radius),
		  rear_wheel_radius_(rear_wheel_radius),
		  cabin_(std::move(cabin)),
		  front_powertrain_(std::move(front_powertrain)),
		  rear_powertrain_(std::move(rear_powertrain)),
		  battery_(std::move(battery)),
		  luggage_(std::move(luggage))
	{
	}

	float overallLength() const { return overall_length_; }
	float overallWidth() const { return overall_width_; }
	float overallHeight() const { return overall_height_; }
	float wheelbase() const { return wheelbase_; }
	float frontTrack() const { return front_track_; }
	float rearTrack() const { return rear_track_; }
	float groundClearance() const { return ground_clearance_; }
	float frontOverhang() const { return front_overhang_; }
	float rearOverhang() const { return rear_overhang_; }
	float frontWheelRadius() const { return front_wheel_radius_; }
	float rearWheelRadius() const { return rear_wheel_radius_; }
	const VehiclePackageEnvelope &cabin() const { return cabin_; }
	const VehiclePackageEnvelope &frontPowertrain() const { return front_powertrain_; }
	const VehiclePackageEnvelope &rearPowertrain() const { return rear_powertrain_; }
	const VehiclePackageEnvelope &battery() const { return battery_; }
	const VehiclePackageEnvelope &luggage() const { return luggage_; }

	float frontBumperStation() const { return front_overhang_; }
	float rearAxleStation() const { return -wheelbase_; }
	float rearBumperStation() const { return -(wheelbase_ + rear_overhang_); }

private:
	float overall_length_ = 0.0f;
	float overall_width_ = 0.0f;
	float overall_height_ = 0.0f;
	float wheelbase_ = 0.0f;
	float front_track_ = 0.0f;
	float rear_track_ = 0.0f;
	float ground_clearance_ = 0.0f;
	float front_overhang_ = 0.0f;
	float rear_overhang_ = 0.0f;
	float front_wheel_radius_ = 0.0f;
	float rear_wheel_radius_ = 0.0f;
	VehiclePackageEnvelope cabin_;
	VehiclePackageEnvelope front_powertrain_;
	VehiclePackageEnvelope rear_powertrain_;
	VehiclePackageEnvelope battery_;
	VehiclePackageEnvelope luggage_;
};
