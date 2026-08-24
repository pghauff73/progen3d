#pragma once

#include "building/model/BuildingFunctionCoverageResult.h"
#include "building/model/BuildingFunctionModel.h"
#include "building/model/BuildingModelSafetyLimits.h"
#include "building/model/BuildingModelValidationReport.h"

#include <vector>

class SpatialBuildingModel;

class BuildingFunctionCoverageEvaluationService {
public:
	BuildingModelValidationReport validate(
		const BuildingFunctionModel &function_model,
		const SpatialBuildingModel &spatial_model,
		const BuildingModelSafetyLimits &limits = BuildingModelSafetyLimits()) const;

	std::vector<BuildingFunctionCoverageResult> evaluate(
		const BuildingFunctionModel &function_model) const;
};
