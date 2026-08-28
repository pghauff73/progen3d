#pragma once

#include "building/model/BuildingModelSafetyLimits.h"
#include "building/model/SmallModernBuildingModelConstructionRequest.h"
#include "building/model/SmallModernBuildingModelConstructionResult.h"

#include <memory>

class SpatialBuildingModel;

class SmallModernBuildingModelConstructionService {
public:
	SmallModernBuildingModelConstructionResult construct(
		const SpatialBuildingModel &spatial_model,
		const SmallModernBuildingModelConstructionRequest &request,
		const BuildingModelSafetyLimits &limits = BuildingModelSafetyLimits()) const;

	SmallModernBuildingModelConstructionResult construct(
		std::shared_ptr<const SpatialBuildingModel> spatial_model,
		const SmallModernBuildingModelConstructionRequest &request,
		const BuildingModelSafetyLimits &limits = BuildingModelSafetyLimits()) const;
};
