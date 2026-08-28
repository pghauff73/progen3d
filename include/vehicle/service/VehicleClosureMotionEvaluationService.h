#pragma once

#include "vehicle/model/VehicleClosureMotion.h"

class VehicleClosureMotionEvaluationService
{
public:
	VehicleClosurePoseSample evaluateClosurePose(
		const VehicleRisingRevoluteJoint &joint,
		double normalized_state) const;

	glm::dmat4 evaluateGlassLocalTransform(
		const VehicleHelicalGlassMotion &motion,
		double normalized_state) const;

	VehicleGlassPoseSample evaluateGlassPose(
		const VehicleHelicalGlassMotion &motion,
		double normalized_state,
		const glm::dmat4 &parent_closure_source_transform) const;
};
