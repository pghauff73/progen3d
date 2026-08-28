#pragma once

#include "vehicle/model/VehicleWheelPoseEvaluation.h"
#include "vehicle/model/VehicleWheelPoseSolutionPolicy.h"

#include <glm/glm.hpp>

class VehicleSuspensionKinematicEvaluationService
{
public:
	VehicleWheelPoseEvaluation evaluateWheelPose(
		VehicleCornerLocation corner,
		const glm::dvec3 &nominal_source_hub_centre,
		double source_track_width_metres,
		double wheel_radius_metres,
		double requested_steering_degrees,
		double suspension_travel_metres,
		const VehicleWheelPoseSolutionPolicy &solution_policy) const;
};
