#pragma once

#include "vehicle/mcsmv2/model/McsMv220SourceRelease.h"
#include "vehicle/mcsmv2/model/McsMv22EvaluatedKinematicState.h"
#include "vehicle/mcsmv2/model/McsMv22KinematicState.h"
#include "vehicle/mcsmv2/model/McsMv22KinematicVariantDefinition.h"
#include "vehicle/mcsmv2/service/McsMv22SuspensionKinematicEvaluationService.h"
#include "vehicle/service/VehicleClosureMotionEvaluationService.h"
#include "vehicle/service/VehicleReferenceFrameTransformationService.h"
#include "vehicle/service/VehicleSuspensionKinematicEvaluationService.h"

class McsMv22KinematicPoseEvaluationService
{
public:
	McsMv22KinematicState createNeutralState(
		const McsMv22KinematicVariantDefinition &variant) const;

	McsMv22EvaluatedKinematicState evaluate(
		const McsMv22KinematicVariantDefinition &variant,
		const McsMv220SourceRelease &source_release,
		const McsMv22KinematicState &state) const;

private:
	VehicleSuspensionKinematicEvaluationService suspension_service_;
	McsMv22SuspensionKinematicEvaluationService mcsm_suspension_service_;
	VehicleClosureMotionEvaluationService closure_service_;
	VehicleReferenceFrameTransformationService frame_service_;
};
