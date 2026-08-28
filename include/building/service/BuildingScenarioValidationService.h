#pragma once

#include "building/model/BuildingModelSafetyLimits.h"
#include "building/model/BuildingModelValidationReport.h"
#include "building/model/BuildingScenarioModel.h"

class SpatialBuildingModel;

class BuildingScenarioValidationService {
public:
	BuildingModelValidationReport validate(
		const BuildingScenarioModel &scenario_model,
		const SpatialBuildingModel &spatial_model,
		const BuildingModelSafetyLimits &limits = BuildingModelSafetyLimits()) const;
};
