#pragma once

#include "building/model/BuildingClassificationModel.h"
#include "building/model/BuildingModelSafetyLimits.h"
#include "building/model/BuildingModelValidationReport.h"

class SpatialBuildingModel;

class BuildingClassificationValidationService {
public:
	BuildingModelValidationReport validate(
		const BuildingClassificationModel &classification_model,
		const SpatialBuildingModel &spatial_model,
		const BuildingModelSafetyLimits &limits = BuildingModelSafetyLimits()) const;
};
