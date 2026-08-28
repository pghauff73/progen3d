#pragma once

#include "vehicle/parametric/model/VehicleWheelParameters.h"

class VehicleWheelMotionParameters
{
public:
	VehicleWheelMotionParameters(
		VehicleWheelParameters wheel_geometry,
		double steer_minimum_degrees,
		double steer_maximum_degrees,
		double travel_minimum,
		double travel_maximum,
		double wheelhouse_clearance)
		: wheel_geometry_(wheel_geometry),
		  steer_minimum_degrees_(steer_minimum_degrees),
		  steer_maximum_degrees_(steer_maximum_degrees),
		  travel_minimum_(travel_minimum),
		  travel_maximum_(travel_maximum),
		  wheelhouse_clearance_(wheelhouse_clearance)
	{
	}

	const VehicleWheelParameters &wheelGeometry() const { return wheel_geometry_; }
	double steerMinimumDegrees() const { return steer_minimum_degrees_; }
	double steerMaximumDegrees() const { return steer_maximum_degrees_; }
	double travelMinimum() const { return travel_minimum_; }
	double travelMaximum() const { return travel_maximum_; }
	double wheelhouseClearance() const { return wheelhouse_clearance_; }

private:
	VehicleWheelParameters wheel_geometry_{0.0, 0.0, 0.0, 0.0};
	double steer_minimum_degrees_ = 0.0;
	double steer_maximum_degrees_ = 0.0;
	double travel_minimum_ = 0.0;
	double travel_maximum_ = 0.0;
	double wheelhouse_clearance_ = 0.0;
};
