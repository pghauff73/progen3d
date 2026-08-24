#include "vehicle/service/VehicleSuspensionKinematicEvaluationService.h"

#include <glm/gtc/matrix_transform.hpp>

namespace {

double lateralSign(VehicleCornerLocation corner)
{
	return corner == VehicleCornerLocation::FrontLeft ||
		       corner == VehicleCornerLocation::RearLeft
		       ? -1.0
		       : 1.0;
}

} // namespace

VehicleWheelPoseEvaluation
VehicleSuspensionKinematicEvaluationService::evaluateWheelPose(
	VehicleCornerLocation corner,
	const glm::dvec3 &nominal_source_hub_centre,
	double source_track_width_metres,
	double wheel_radius_metres,
	double requested_steering_degrees,
	double suspension_travel_metres,
	const VehicleWheelPoseSolutionPolicy &solution_policy) const
{
	const double side_sign = lateralSign(corner);
	const double steering_degrees = solution_policy.steeringEnabled()
		                                ? requested_steering_degrees
		                                : 0.0;
	const double camber_degrees =
		solution_policy.staticCamberDegrees() +
		solution_policy.camberGainDegreesPerMetre() * suspension_travel_metres;
	const double toe_degrees =
		solution_policy.toeGainDegreesPerMetre() * suspension_travel_metres;
	const glm::dvec3 source_centre(
		nominal_source_hub_centre.x +
			solution_policy.longitudinalCentreGain() * suspension_travel_metres,
		side_sign *
			(source_track_width_metres * 0.5 +
			 solution_policy.lateralTrackGain() * suspension_travel_metres),
		wheel_radius_metres + suspension_travel_metres);

	const glm::dmat4 translation =
		glm::translate(glm::dmat4(1.0), source_centre);
	const glm::dmat4 steering_and_toe_rotation = glm::rotate(
		glm::dmat4(1.0), glm::radians(steering_degrees + toe_degrees),
		glm::dvec3(0.0, 0.0, 1.0));
	const glm::dmat4 camber_rotation = glm::rotate(
		glm::dmat4(1.0), glm::radians(side_sign * camber_degrees),
		glm::dvec3(1.0, 0.0, 0.0));

	return VehicleWheelPoseEvaluation(
		corner, steering_degrees, suspension_travel_metres, camber_degrees,
		toe_degrees,
		source_centre,
		translation * steering_and_toe_rotation * camber_rotation);
}
