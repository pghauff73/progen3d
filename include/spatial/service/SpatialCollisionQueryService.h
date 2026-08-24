#pragma once

#include "Context.h"
#include "spatial/model/CollisionPositionConstraint.h"
#include "spatial/model/SpatialBuildingModel.h"
#include "spatial/model/SpatialRelationResult.h"

#include <vector>

class SpatialCollisionQueryService {
public:
	bool targetContactIsPermitted(
		const SpatialBuildingObject &moving_object,
		const SpatialBuildingObject &target_object,
		const CollisionLayerMask &constraint_mask) const;

	SpatialRelationResult query(
		const SpatialBuildingObject &first_object,
		const SpatialBuildingObject &second_object,
		const SpatialBuildingModel &model,
		const SceneGenerationContext &scene_context,
		const std::vector<ScenePrimitiveInstance> *primitive_instances_override = nullptr) const;

	std::vector<SpatialObjectId> findForbiddenOverlaps(
		const SpatialBuildingObject &moving_object,
		const glm::mat4 &candidate_world_transform,
		const SpatialObjectId &permitted_target_id,
		const CollisionLayerMask &constraint_mask,
		const SpatialBuildingModel &model,
		const SceneGenerationContext &scene_context,
		const std::vector<ScenePrimitiveInstance> *primitive_instances_override,
		float tolerance,
		int *evaluated_boundary_pair_count = nullptr) const;
};
