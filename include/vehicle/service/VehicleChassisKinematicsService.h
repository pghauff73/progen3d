#pragma once

#include "vehicle/model/VehicleChassisMvp25.h"
#include "vehicle/model/VehicleValidationReport.h"

#include <cstddef>

class WheelPoseSolver
{
public:
	SolvedWheelPose solve(
		const SuspensionHardpointModel &hardpoint_model,
		const WheelPoseState &state) const;
};

class VehicleChassisKinematicsService
{
public:
	WheelPoseFunction sampleWheelPoseFunction(
		const SuspensionHardpointModel &hardpoint_model,
		std::size_t travel_sample_count,
		std::size_t steering_sample_count) const;

	WheelSweptEnvelope calculateWheelSweptEnvelope(
		const WheelPoseFunction &pose_function,
		const TyreGeometry &tyre) const;

	bool wheelHouseContainsEnvelope(
		const WheelHouse &wheel_house,
		const WheelSweptEnvelope &envelope,
		float tolerance) const;

	VehicleValidationReport validateHardpointModel(
		const SuspensionHardpointModel &hardpoint_model) const;
};
