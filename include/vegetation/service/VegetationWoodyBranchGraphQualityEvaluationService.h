#pragma once

#include "vegetation/model/VegetationWoodyBranchGraphQualityEvidence.h"
#include "vegetation/model/VegetationWoodyBranchGraphQualityReport.h"
#include "vegetation/model/VegetationWoodyBranchGraphReconstructionPolicy.h"

class VegetationWoodyBranchGraphQualityEvaluationService
{
public:
	VegetationWoodyBranchGraphQualityReport evaluate(
		const VegetationWoodyBranchGraphQualityEvidence &evidence,
		const VegetationWoodyBranchGraphReconstructionPolicy &policy) const;
};
