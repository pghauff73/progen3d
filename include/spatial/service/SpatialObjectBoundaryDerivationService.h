#pragma once

#include "spatial/model/SpatialBoundaryModel.h"
#include "spatial/model/SpatialBuildingObject.h"
#include "spatial/relationship/SpatialObjectGeometryBinding.h"

#include <vector>

#include <glm/glm.hpp>

class SceneGenerationContext;
class ScenePrimitiveInstance;

class SpatialObjectBoundaryDerivationService {
public:
	SpatialBoundaryModel derive(
		const SpatialBuildingObject &object,
		const std::vector<SpatialObjectGeometryBinding> &bindings,
		const SceneGenerationContext &scene_context) const;

	SpatialBoundaryModel deriveAtWorldTransform(
		const SpatialBuildingObject &object,
		const std::vector<SpatialObjectGeometryBinding> &bindings,
		const SceneGenerationContext &scene_context,
		const glm::mat4 &candidate_world_transform,
		const std::vector<ScenePrimitiveInstance> *primitive_instances_override = nullptr) const;
};
