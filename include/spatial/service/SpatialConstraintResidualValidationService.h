#pragma once

#include "Context.h"
#include "spatial/model/CollisionPositionConstraint.h"
#include "spatial/model/SpatialBuildingModel.h"
#include "spatial/model/SpatialModelValidationReport.h"
#include "spatial/model/SpatialPositioningSafetyLimits.h"

#include <vector>

class SpatialConstraintResidualValidationService {
public:
	SpatialModelValidationReport validate(
		const CollisionPositionConstraint &constraint,
		const SpatialBuildingModel &model,
		const SceneGenerationContext &scene_context,
		const std::vector<ScenePrimitiveInstance> *primitive_instances_override = nullptr,
		const SpatialPositioningSafetyLimits &limits =
			SpatialPositioningSafetyLimits()) const;
};
