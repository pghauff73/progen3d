#pragma once

#include "vegetation/model/VegetationWoodySegmentationCandidateEvaluation.h"
#include "vegetation/model/VegetationWoodySegmentationSensitivityPolicy.h"
#include "vegetation/model/VegetationWoodySegmentationSensitivityReport.h"

#include <vector>

class VegetationWoodySegmentationSensitivityEvaluationService
{
public:
	VegetationWoodySegmentationSensitivityReport evaluate(
		const std::vector<VegetationWoodySegmentationCandidateEvaluation>
			&evaluations,
		const VegetationWoodySegmentationSensitivityPolicy &policy) const;
};
