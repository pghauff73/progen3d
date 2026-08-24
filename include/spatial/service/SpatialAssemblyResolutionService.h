#pragma once

#include "Context.h"
#include "spatial/model/SpatialAssemblyResolutionResult.h"
#include "spatial/model/SpatialBuildingModel.h"
#include "spatial/model/SpatialPositioningSafetyLimits.h"

class SpatialAssemblyResolutionService {
public:
	SpatialAssemblyResolutionResult resolve(
		SpatialBuildingModel construction_model,
		SceneGenerationContext *scene_context,
		const SpatialPositioningSafetyLimits &limits =
			SpatialPositioningSafetyLimits()) const;
};
