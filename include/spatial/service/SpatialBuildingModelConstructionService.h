#pragma once

#include "spatial/model/SpatialBuildingModelConstructionRequest.h"
#include "spatial/model/SpatialBuildingModelConstructionResult.h"
#include "spatial/model/SpatialModelSafetyLimits.h"

class SpatialBuildingModelConstructionService {
public:
	SpatialBuildingModelConstructionResult construct(
		const SpatialBuildingModelConstructionRequest &request,
		const SpatialModelSafetyLimits &limits = SpatialModelSafetyLimits()) const;
};
