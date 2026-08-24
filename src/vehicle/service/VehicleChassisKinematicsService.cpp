#include "vehicle/service/VehicleChassisKinematicsService.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace {

AxisAlignedBounds tyre_bounds(
	const SolvedWheelPose &pose,
	const TyreGeometry &tyre)
{
	AxisAlignedBounds bounds;
	const glm::vec3 half_extents(
		tyre.sectionWidth() * 0.5f,
		tyre.unloadedRadius(),
		tyre.unloadedRadius());
	bounds.min = pose.centre() - half_extents;
	bounds.max = pose.centre() + half_extents;
	bounds.center = pose.centre();
	bounds.half_extents = half_extents;
	bounds.valid = true;
	return bounds;
}

AxisAlignedBounds merge_bounds(
	const AxisAlignedBounds &first,
	const AxisAlignedBounds &second)
{
	if (!first.valid) return second;
	if (!second.valid) return first;
	AxisAlignedBounds merged;
	merged.min = glm::min(first.min, second.min);
	merged.max = glm::max(first.max, second.max);
	merged.center = (merged.min + merged.max) * 0.5f;
	merged.half_extents = (merged.max - merged.min) * 0.5f;
	merged.valid = true;
	return merged;
}

} // namespace

SolvedWheelPose WheelPoseSolver::solve(
	const SuspensionHardpointModel &hardpoint_model,
	const WheelPoseState &state) const
{
	const SuspensionHardpoint *wheel_centre =
		hardpoint_model.findHardpoint(SuspensionHardpointRole::WheelCentre);
	const glm::vec3 nominal_centre = wheel_centre != nullptr
		? wheel_centre->position()
		: glm::vec3(0.0f);
	const float travel = glm::clamp(
		state.suspensionTravel(), hardpoint_model.minimumTravel(),
		hardpoint_model.maximumTravel());
	const float steering = glm::clamp(
		state.steeringDegrees(), hardpoint_model.minimumSteeringDegrees(),
		hardpoint_model.maximumSteeringDegrees());
	const glm::mat4 steering_rotation = glm::rotate(
		glm::mat4(1.0f), glm::radians(steering), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::vec3 axle_direction = glm::normalize(glm::vec3(
		steering_rotation * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
	const float normalized_travel = hardpoint_model.maximumTravel() >
		                               hardpoint_model.minimumTravel()
		? (travel - hardpoint_model.minimumTravel()) /
		  (hardpoint_model.maximumTravel() - hardpoint_model.minimumTravel())
		: 0.5f;
	return SolvedWheelPose(
		WheelPoseState(travel, steering),
		nominal_centre + glm::vec3(0.0f, travel, 0.0f),
		axle_direction,
		-1.5f - 1.2f * (normalized_travel - 0.5f),
		steering * 0.04f);
}

WheelPoseFunction VehicleChassisKinematicsService::sampleWheelPoseFunction(
	const SuspensionHardpointModel &hardpoint_model,
	std::size_t travel_sample_count,
	std::size_t steering_sample_count) const
{
	const std::size_t travel_count = std::max<std::size_t>(travel_sample_count, 2u);
	const std::size_t steering_count = std::max<std::size_t>(steering_sample_count, 1u);
	std::vector<SolvedWheelPose> poses;
	poses.reserve(travel_count * steering_count);
	for (std::size_t travel_index = 0u; travel_index < travel_count; ++travel_index) {
		const float travel_parameter = static_cast<float>(travel_index) /
			static_cast<float>(travel_count - 1u);
		const float travel = glm::mix(
			hardpoint_model.minimumTravel(), hardpoint_model.maximumTravel(),
			travel_parameter);
		for (std::size_t steering_index = 0u; steering_index < steering_count;
		     ++steering_index) {
			const float steering_parameter = steering_count > 1u
				? static_cast<float>(steering_index) /
				  static_cast<float>(steering_count - 1u)
				: 0.5f;
			const float steering = glm::mix(
				hardpoint_model.minimumSteeringDegrees(),
				hardpoint_model.maximumSteeringDegrees(), steering_parameter);
			poses.push_back(WheelPoseSolver().solve(
				hardpoint_model, WheelPoseState(travel, steering)));
		}
	}
	return WheelPoseFunction(
		hardpoint_model.identifier() + ".WheelPoseFunction",
		hardpoint_model.identifier(), std::move(poses));
}

WheelSweptEnvelope VehicleChassisKinematicsService::calculateWheelSweptEnvelope(
	const WheelPoseFunction &pose_function,
	const TyreGeometry &tyre) const
{
	std::vector<AxisAlignedBounds> samples;
	AxisAlignedBounds swept;
	for (const SolvedWheelPose &pose : pose_function.sampledPoses()) {
		const AxisAlignedBounds sample = tyre_bounds(pose, tyre);
		samples.push_back(sample);
		swept = merge_bounds(swept, sample);
	}
	return WheelSweptEnvelope(
		pose_function.identifier() + ".SweptEnvelope", std::move(samples), swept);
}

bool VehicleChassisKinematicsService::wheelHouseContainsEnvelope(
	const WheelHouse &wheel_house,
	const WheelSweptEnvelope &envelope,
	float tolerance) const
{
	const AxisAlignedBounds &house = wheel_house.cavityBounds();
	const AxisAlignedBounds &swept = envelope.sweptBounds();
	const float clearance = wheel_house.minimumClearance() - tolerance;
	return house.valid && swept.valid &&
	       swept.min.x >= house.min.x + clearance &&
	       swept.min.y >= house.min.y + clearance &&
	       swept.min.z >= house.min.z + clearance &&
	       swept.max.x <= house.max.x - clearance &&
	       swept.max.y <= house.max.y - clearance &&
	       swept.max.z <= house.max.z - clearance;
}

VehicleValidationReport VehicleChassisKinematicsService::validateHardpointModel(
	const SuspensionHardpointModel &hardpoint_model) const
{
	VehicleValidationReport report;
	std::unordered_set<std::string> hardpoint_identifiers;
	for (const SuspensionHardpoint &hardpoint : hardpoint_model.hardpoints()) {
		if (hardpoint.identifier().empty() ||
		    !hardpoint_identifiers.insert(hardpoint.identifier()).second ||
		    !std::isfinite(hardpoint.position().x) ||
		    !std::isfinite(hardpoint.position().y) ||
		    !std::isfinite(hardpoint.position().z)) {
			report.addIssue(VehicleValidationIssue(
				VehicleDiagnosticCode::InvalidJoint,
				"Suspension hardpoints require unique identifiers and finite positions.",
				{hardpoint.identifier()}));
		}
	}
	if (hardpoint_model.findHardpoint(SuspensionHardpointRole::WheelCentre) == nullptr) {
		report.addIssue(VehicleValidationIssue(
			VehicleDiagnosticCode::InvalidJoint,
			"Suspension hardpoint model requires a wheel-centre hardpoint.",
			{hardpoint_model.identifier()}));
	}
	for (const KinematicLink &link : hardpoint_model.links()) {
		if (hardpoint_identifiers.count(link.firstHardpointIdentifier()) == 0u ||
		    hardpoint_identifiers.count(link.secondHardpointIdentifier()) == 0u ||
		    !std::isfinite(link.nominalLength()) || link.nominalLength() <= 0.0f) {
			report.addIssue(VehicleValidationIssue(
				VehicleDiagnosticCode::InvalidJoint,
				"Kinematic links must reference existing hardpoints and have positive length.",
				{link.identifier()}));
		}
	}
	return report;
}
