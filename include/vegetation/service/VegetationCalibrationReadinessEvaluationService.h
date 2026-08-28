#pragma once

#include "vegetation/model/BuildingVegetationObjectModel.h"
#include "vegetation/model/VegetationCalibrationDomain.h"
#include "vegetation/model/VegetationCalibrationReadinessReport.h"

#include <vector>

class VegetationCalibrationReadinessEvaluationService
{
public:
	VegetationCalibrationReadinessReport evaluate(
		const BuildingVegetationObjectModel &object_model,
		const std::vector<VegetationCalibrationDomain> &requested_domains) const;
};
