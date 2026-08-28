#pragma once

#include "vehicle/model/SuspensionCornerSpecification.h"

#include <glm/glm.hpp>

class VehicleWheelPoseEvaluation
{
public:
	VehicleWheelPoseEvaluation(
		VehicleCornerLocation corner,
		double steering_degrees,
		double suspension_travel_metres,
		double camber_degrees,
		double toe_degrees,
		glm::dvec3 source_centre,
		glm::dmat4 source_transform)
		: corner_(corner),
		  steering_degrees_(steering_degrees),
		  suspension_travel_metres_(suspension_travel_metres),
		  camber_degrees_(camber_degrees),
		  toe_degrees_(toe_degrees),
		  source_centre_(source_centre),
		  source_transform_(source_transform)
	{
	}

	VehicleCornerLocation corner() const { return corner_; }
	double steeringDegrees() const { return steering_degrees_; }
	double suspensionTravelMetres() const
	{
		return suspension_travel_metres_;
	}
	double camberDegrees() const { return camber_degrees_; }
	double toeDegrees() const { return toe_degrees_; }
	const glm::dvec3 &sourceCentre() const { return source_centre_; }
	const glm::dmat4 &sourceTransform() const { return source_transform_; }

private:
	VehicleCornerLocation corner_ = VehicleCornerLocation::FrontLeft;
	double steering_degrees_ = 0.0;
	double suspension_travel_metres_ = 0.0;
	double camber_degrees_ = 0.0;
	double toe_degrees_ = 0.0;
	glm::dvec3 source_centre_{0.0};
	glm::dmat4 source_transform_{1.0};
};
