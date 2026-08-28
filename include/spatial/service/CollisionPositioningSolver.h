#pragma once

#include "Context.h"
#include "spatial/model/CollisionPositionConstraint.h"
#include "spatial/model/SpatialBuildingModel.h"
#include "spatial/model/SpatialConstraintResolutionResult.h"
#include "spatial/model/SpatialPositioningSafetyLimits.h"

#include <vector>

class CollisionPositioningSolver {
public:
	SpatialConstraintResolutionResult solve(
		const CollisionPositionConstraint &constraint,
		const SpatialBuildingModel &model,
		const SceneGenerationContext &scene_context,
		const std::vector<ScenePrimitiveInstance> *primitive_instances_override = nullptr,
		const SpatialPositioningSafetyLimits &limits =
			SpatialPositioningSafetyLimits()) const;
};
