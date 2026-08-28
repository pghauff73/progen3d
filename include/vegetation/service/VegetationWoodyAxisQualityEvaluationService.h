#pragma once

#include "vegetation/model/VegetationWoodyAxisQualityEvidence.h"
#include "vegetation/model/VegetationWoodyAxisQualityReport.h"
#include "vegetation/model/VegetationWoodyAxisReconstructionPolicy.h"

class VegetationWoodyAxisQualityEvaluationService
{
public:
	VegetationWoodyAxisQualityReport evaluate(
		const VegetationWoodyAxisQualityEvidence &evidence,
		const VegetationWoodyAxisReconstructionPolicy &policy) const;
};
