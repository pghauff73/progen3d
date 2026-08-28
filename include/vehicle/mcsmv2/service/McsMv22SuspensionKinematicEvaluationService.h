#pragma once

#include "vehicle/mcsmv2/model/McsMv22KinematicVariantDefinition.h"
#include "vehicle/model/VehicleSuspensionCornerKinematicModel.h"
#include "vehicle/model/VehicleWheelPoseEvaluation.h"
#include "vehicle/service/VehicleSuspensionKinematicEvaluationService.h"

#include <vector>

class McsMv22SuspensionKinematicEvaluationService
{
public:
	explicit McsMv22SuspensionKinematicEvaluationService(
		VehicleSuspensionKinematicEvaluationService evaluation_service = {});

	VehicleWheelPoseEvaluation evaluateAcceptedPose(
		const McsMv22KinematicVariantDefinition &variant,
		const McsMv22WheelPoseEvidence &accepted_pose) const;

	std::vector<VehicleWheelPoseEvaluation> evaluateAcceptedPoseGrid(
		const McsMv22KinematicVariantDefinition &variant) const;

	std::vector<VehicleSuspensionCornerKinematicModel> createCornerModels(
		const McsMv22KinematicVariantDefinition &variant) const;

private:
	const McsMv22SuspensionHardpointDefinition &findHubCentre(
		const McsMv22KinematicVariantDefinition &variant,
		const McsMv22WheelPoseEvidence &accepted_pose) const;

	VehicleCornerLocation interpretCorner(
		const std::string &wheel_identifier) const;

	VehicleWheelPoseSolutionPolicy createSolutionPolicy(
		VehicleCornerLocation corner) const;

	VehicleSuspensionKinematicEvaluationService evaluation_service_;
};
