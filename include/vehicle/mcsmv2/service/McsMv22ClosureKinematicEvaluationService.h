#pragma once

#include "vehicle/mcsmv2/model/McsMv22KinematicVariantDefinition.h"
#include "vehicle/model/VehicleClosureMotion.h"
#include "vehicle/model/VehicleJointGraph.h"
#include "vehicle/service/VehicleClosureMotionEvaluationService.h"

class McsMv22ClosureKinematicEvaluationService
{
public:
	explicit McsMv22ClosureKinematicEvaluationService(
		VehicleClosureMotionEvaluationService motion_service = {});

	VehicleClosureSweepSet createClosureSweepSet(
		const McsMv22KinematicVariantDefinition &variant) const;

	VehicleGlassMotionSet createGlassMotionSet(
		const McsMv22KinematicVariantDefinition &variant,
		double parent_closure_state) const;

	VehicleJointGraph createJointGraph(
		const McsMv22KinematicVariantDefinition &variant) const;

private:
	VehicleClosureSurfaceBinding createClosureBinding(
		const McsMv22KinematicVariantDefinition &variant,
		const McsMv22ClosureHingeDefinition &hinge) const;

	VehicleGlassSurfaceBinding createGlassBinding(
		const McsMv22KinematicVariantDefinition &variant,
		const McsMv22HelicalGlassDefinition &glass) const;

	const McsMv22ClosureHingeDefinition &findParentHinge(
		const McsMv22KinematicVariantDefinition &variant,
		const std::string &closure_identifier) const;

	VehicleClosureMotionEvaluationService motion_service_;
};
