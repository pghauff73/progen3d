#pragma once

#include "vegetation/model/VegetationCanopyOccupancyQualityEvidence.h"
#include "vegetation/model/VegetationCanopyOccupancyQualityReport.h"
#include "vegetation/model/VegetationCanopyOccupancyReconstructionPolicy.h"

class VegetationCanopyOccupancyQualityEvaluationService
{
public:
	VegetationCanopyOccupancyQualityReport evaluate(
		const VegetationCanopyOccupancyQualityEvidence &evidence,
		const VegetationCanopyOccupancyReconstructionPolicy &policy) const;
};
