#include "vehicle/mcsmv2/service/McsMv22SuspensionKinematicEvaluationService.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

McsMv22SuspensionKinematicEvaluationService::
	McsMv22SuspensionKinematicEvaluationService(
		VehicleSuspensionKinematicEvaluationService evaluation_service)
	: evaluation_service_(std::move(evaluation_service))
{
}

VehicleWheelPoseEvaluation
McsMv22SuspensionKinematicEvaluationService::evaluateAcceptedPose(
	const McsMv22KinematicVariantDefinition &variant,
	const McsMv22WheelPoseEvidence &accepted_pose) const
{
	const VehicleCornerLocation corner =
		interpretCorner(accepted_pose.wheelIdentifier());
	const McsMv22SuspensionHardpointDefinition &hub_centre =
		findHubCentre(variant, accepted_pose);
	return evaluation_service_.evaluateWheelPose(
		corner, hub_centre.sourcePosition(),
		2.0 * std::abs(hub_centre.sourcePosition().y),
		variant.wheelRadiusMetres(), accepted_pose.steerDegrees(),
		accepted_pose.travelMetres(), createSolutionPolicy(corner));
}

std::vector<VehicleWheelPoseEvaluation>
McsMv22SuspensionKinematicEvaluationService::evaluateAcceptedPoseGrid(
	const McsMv22KinematicVariantDefinition &variant) const
{
	std::vector<VehicleWheelPoseEvaluation> evaluations;
	evaluations.reserve(variant.acceptedWheelPoses().size());
	for (const McsMv22WheelPoseEvidence &accepted_pose :
	     variant.acceptedWheelPoses()) {
		evaluations.push_back(evaluateAcceptedPose(variant, accepted_pose));
	}
	return evaluations;
}

std::vector<VehicleSuspensionCornerKinematicModel>
McsMv22SuspensionKinematicEvaluationService::createCornerModels(
	const McsMv22KinematicVariantDefinition &variant) const
{
	const std::pair<const char *, VehicleCornerLocation> corners[] = {
		{"front_left", VehicleCornerLocation::FrontLeft},
		{"front_right", VehicleCornerLocation::FrontRight},
		{"rear_left", VehicleCornerLocation::RearLeft},
		{"rear_right", VehicleCornerLocation::RearRight}};
	std::vector<VehicleSuspensionCornerKinematicModel> models;
	models.reserve(4u);
	for (const auto &corner : corners) {
		const std::string identifier = std::string(corner.first) + "_hub_centre";
		const McsMv22SuspensionHardpointDefinition *hub_centre = nullptr;
		for (const McsMv22SuspensionHardpointDefinition &hardpoint :
		     variant.hardpoints()) {
			if (hardpoint.identifier() == identifier &&
			    hardpoint.role() == "hub_centre") {
				hub_centre = &hardpoint;
				break;
			}
		}
		if (hub_centre == nullptr) {
			throw std::runtime_error(
				"MCSMv2.2 suspension corner has no nominal hub centre: " + identifier);
		}
		const bool front = corner.second == VehicleCornerLocation::FrontLeft ||
		                   corner.second == VehicleCornerLocation::FrontRight;
		models.emplace_back(
			corner.second, hub_centre->sourcePosition(),
			2.0 * std::abs(hub_centre->sourcePosition().y),
			variant.wheelRadiusMetres(),
			front ? variant.minimumSteerDegrees() : 0.0,
			front ? variant.maximumSteerDegrees() : 0.0,
			variant.minimumTravelMetres(), variant.maximumTravelMetres(),
			createSolutionPolicy(corner.second));
	}
	return models;
}

const McsMv22SuspensionHardpointDefinition &
McsMv22SuspensionKinematicEvaluationService::findHubCentre(
	const McsMv22KinematicVariantDefinition &variant,
	const McsMv22WheelPoseEvidence &accepted_pose) const
{
	for (const McsMv22SuspensionHardpointDefinition &hardpoint :
	     variant.hardpoints()) {
		if (hardpoint.role() == "hub_centre" &&
		    hardpoint.identifier() == accepted_pose.wheelIdentifier() + "_hub_centre") {
			return hardpoint;
		}
	}
	throw std::runtime_error(
		"MCSMv2.2 accepted wheel pose has no matching hub centre: " +
		accepted_pose.wheelIdentifier());
}

VehicleCornerLocation
McsMv22SuspensionKinematicEvaluationService::interpretCorner(
	const std::string &wheel_identifier) const
{
	if (wheel_identifier == "front_left") return VehicleCornerLocation::FrontLeft;
	if (wheel_identifier == "front_right") return VehicleCornerLocation::FrontRight;
	if (wheel_identifier == "rear_left") return VehicleCornerLocation::RearLeft;
	if (wheel_identifier == "rear_right") return VehicleCornerLocation::RearRight;
	throw std::runtime_error(
		"Unsupported MCSMv2.2 wheel identifier: " + wheel_identifier);
}

VehicleWheelPoseSolutionPolicy
McsMv22SuspensionKinematicEvaluationService::createSolutionPolicy(
	VehicleCornerLocation corner) const
{
	const bool front = corner == VehicleCornerLocation::FrontLeft ||
	                   corner == VehicleCornerLocation::FrontRight;
	return VehicleWheelPoseSolutionPolicy(
		front, front ? -1.0 : -1.3, front ? -11.0 : -14.0,
		front ? 1.5 : 2.0, front ? -0.025 : 0.010, 0.018);
}
