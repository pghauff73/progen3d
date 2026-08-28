#include "vehicle/mcsmv2/service/VehicleWheelMotionEnvelopeConstructionService.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

VehicleWheelEnvelopeSet VehicleWheelMotionEnvelopeConstructionService::construct(
	const ModernCarSemanticVariant &variant) const
{
	const VehicleWheelMotionParameters &motion = variant.wheelMotion();
	const VehicleWheelParameters &wheels = motion.wheelGeometry();
	const double steering_angle = std::max(
		std::fabs(motion.steerMinimumDegrees()),
		std::fabs(motion.steerMaximumDegrees()));
	const double steering_sine = std::sin(steering_angle * 3.14159265358979323846 / 180.0);
	const double vertical_travel = std::max(
		std::fabs(motion.travelMinimum()),
		std::fabs(motion.travelMaximum()));
	const glm::dvec3 front_radii(
		wheels.radius() + 0.5 * wheels.width() * steering_sine +
			motion.wheelhouseClearance(),
		0.5 * wheels.width() + wheels.radius() * steering_sine +
			motion.wheelhouseClearance(),
		wheels.radius() + vertical_travel + motion.wheelhouseClearance());
	const glm::dvec3 rear_radii(
		wheels.radius() + motion.wheelhouseClearance(),
		0.5 * wheels.width() + motion.wheelhouseClearance(),
		wheels.radius() + vertical_travel + motion.wheelhouseClearance());
	const double front_x = variant.package().wheelbase() * 0.5;
	const double rear_x = -variant.package().wheelbase() * 0.5;

	return VehicleWheelEnvelopeSet({{
		VehicleWheelMotionEnvelope(
			variant.identifier() + ".FrontLeftWheelEnvelope",
			{VehicleAxleRole::Front, VehicleSideRole::Left},
			{front_x, -wheels.frontTrack() * 0.5, wheels.radius()},
			front_radii,
			{motion.steerMinimumDegrees(), motion.steerMaximumDegrees()},
			{motion.travelMinimum(), motion.travelMaximum()},
			motion.wheelhouseClearance()),
		VehicleWheelMotionEnvelope(
			variant.identifier() + ".FrontRightWheelEnvelope",
			{VehicleAxleRole::Front, VehicleSideRole::Right},
			{front_x, wheels.frontTrack() * 0.5, wheels.radius()},
			front_radii,
			{motion.steerMinimumDegrees(), motion.steerMaximumDegrees()},
			{motion.travelMinimum(), motion.travelMaximum()},
			motion.wheelhouseClearance()),
		VehicleWheelMotionEnvelope(
			variant.identifier() + ".RearLeftWheelEnvelope",
			{VehicleAxleRole::Rear, VehicleSideRole::Left},
			{rear_x, -wheels.rearTrack() * 0.5, wheels.radius()},
			rear_radii,
			{0.0, 0.0},
			{motion.travelMinimum(), motion.travelMaximum()},
			motion.wheelhouseClearance()),
		VehicleWheelMotionEnvelope(
			variant.identifier() + ".RearRightWheelEnvelope",
			{VehicleAxleRole::Rear, VehicleSideRole::Right},
			{rear_x, wheels.rearTrack() * 0.5, wheels.radius()},
			rear_radii,
			{0.0, 0.0},
			{motion.travelMinimum(), motion.travelMaximum()},
			motion.wheelhouseClearance())
	}});
}
