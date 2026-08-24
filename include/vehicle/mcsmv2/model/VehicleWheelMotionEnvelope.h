#pragma once

#include "vehicle/mcsmv2/model/VehicleWheelPositionRelationship.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>

class VehicleWheelMotionEnvelope
{
public:
	VehicleWheelMotionEnvelope(
		std::string identifier,
		VehicleWheelPositionRelationship position,
		glm::dvec3 source_center,
		glm::dvec3 source_radii,
		glm::dvec2 steer_range_degrees,
		glm::dvec2 travel_range,
		double clearance)
		: identifier_(std::move(identifier)),
		  position_(position),
		  source_center_(source_center),
		  source_radii_(source_radii),
		  steer_range_degrees_(steer_range_degrees),
		  travel_range_(travel_range),
		  clearance_(clearance)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const VehicleWheelPositionRelationship &position() const { return position_; }
	const glm::dvec3 &sourceCenter() const { return source_center_; }
	const glm::dvec3 &sourceRadii() const { return source_radii_; }
	const glm::dvec2 &steerRangeDegrees() const { return steer_range_degrees_; }
	const glm::dvec2 &travelRange() const { return travel_range_; }
	double clearance() const { return clearance_; }

private:
	std::string identifier_;
	VehicleWheelPositionRelationship position_{
		VehicleAxleRole::Front, VehicleSideRole::Left};
	glm::dvec3 source_center_{0.0};
	glm::dvec3 source_radii_{0.0};
	glm::dvec2 steer_range_degrees_{0.0};
	glm::dvec2 travel_range_{0.0};
	double clearance_ = 0.0;
};
