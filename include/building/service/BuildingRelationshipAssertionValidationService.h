#pragma once

#include "building/model/BuildingModelSafetyLimits.h"
#include "building/model/BuildingModelValidationReport.h"
#include "building/model/BuildingRelationshipAssertionModel.h"

class SpatialBuildingModel;

class BuildingRelationshipAssertionValidationService {
public:
	BuildingModelValidationReport validate(
		const BuildingRelationshipAssertionModel &assertion_model,
		const SpatialBuildingModel &spatial_model,
		const BuildingModelSafetyLimits &limits = BuildingModelSafetyLimits()) const;
};
