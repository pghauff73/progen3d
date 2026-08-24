#pragma once

#include "vehicle/model/VehicleClassASurface.h"
#include "vehicle/model/VehicleValidationReport.h"

class HighlightFlowValidator
{
public:
	HighlightFlowReport validate(const ClassASurfaceGraph &graph) const;
};

class VehicleClassAValidationService
{
public:
	VehicleValidationReport validateGraph(const ClassASurfaceGraph &graph) const;
	float calculateComplexityPenalty(
		const HighlightFlowReport &report,
		float patch_weight,
		float control_point_weight,
		float span_weight) const;
};
