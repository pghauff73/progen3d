#pragma once

#include "building/model/BuildingModelSafetyLimits.h"
#include "building/model/BuildingModelValidationReport.h"
#include "building/model/SmallModernBuildingModelConstructionRequest.h"

class SpatialBuildingModel;

class SmallModernBuildingModelValidationService {
public:
	BuildingModelValidationReport validate(
		const SpatialBuildingModel &spatial_model,
		const SmallModernBuildingModelConstructionRequest &request,
		const BuildingModelSafetyLimits &limits = BuildingModelSafetyLimits()) const;
	BuildingModelValidationReport validate(
		const SpatialBuildingModel &spatial_model,
		const SmallModernBuildingModelConstructionRequest &request,
		const BuildingModelSafetyLimits &limits,
		double *requirement_evaluation_milliseconds) const;
};
