#include "spatial/service/SpatialCollisionQueryService.h"

#include "spatial/service/SpatialAabbQueryService.h"
#include "spatial/service/SpatialDistanceQueryService.h"
#include "spatial/service/SpatialObjectBoundaryDerivationService.h"

bool SpatialCollisionQueryService::targetContactIsPermitted(
	const SpatialBuildingObject &moving_object,
	const SpatialBuildingObject &target_object,
	const CollisionLayerMask &constraint_mask) const
{
	return constraint_mask.contains(target_object.collisionPolicy().layer()) &&
	       moving_object.collisionPolicy().blocks(target_object.collisionPolicy());
}

SpatialRelationResult SpatialCollisionQueryService::query(
	const SpatialBuildingObject &first_object,
	const SpatialBuildingObject &second_object,
	const SpatialBuildingModel &model,
	const SceneGenerationContext &scene_context,
	const std::vector<ScenePrimitiveInstance> *primitive_instances_override) const
{
	const SpatialObjectBoundaryDerivationService derivation;
	const SpatialBoundaryModel first_boundary = derivation.deriveAtWorldTransform(
		first_object,
		model.bindingsFor(first_object.identity().objectId()),
		scene_context,
		first_object.frameState().resolvedWorldTransform(),
		primitive_instances_override);
	const SpatialBoundaryModel second_boundary = derivation.deriveAtWorldTransform(
		second_object,
		model.bindingsFor(second_object.identity().objectId()),
		scene_context,
		second_object.frameState().resolvedWorldTransform(),
		primitive_instances_override);
	const AxisAlignedBoundingBoundary *first_aabb = first_boundary.axisAlignedBoundary();
	const AxisAlignedBoundingBoundary *second_aabb = second_boundary.axisAlignedBoundary();
	return SpatialDistanceQueryService().measure(
		first_object.identity().objectId(),
		first_aabb != nullptr ? first_aabb->bounds() : AxisAlignedBounds(),
		second_object.identity().objectId(),
		second_aabb != nullptr ? second_aabb->bounds() : AxisAlignedBounds());
}

std::vector<SpatialObjectId> SpatialCollisionQueryService::findForbiddenOverlaps(
	const SpatialBuildingObject &moving_object,
	const glm::mat4 &candidate_world_transform,
	const SpatialObjectId &permitted_target_id,
	const CollisionLayerMask &constraint_mask,
	const SpatialBuildingModel &model,
	const SceneGenerationContext &scene_context,
	const std::vector<ScenePrimitiveInstance> *primitive_instances_override,
	float tolerance,
	int *evaluated_boundary_pair_count) const
{
	if (evaluated_boundary_pair_count != nullptr) *evaluated_boundary_pair_count = 0;
	const SpatialObjectBoundaryDerivationService derivation;
	const SpatialBoundaryModel moving_boundary = derivation.deriveAtWorldTransform(
		moving_object,
		model.bindingsFor(moving_object.identity().objectId()),
		scene_context,
		candidate_world_transform,
		primitive_instances_override);
	const AxisAlignedBoundingBoundary *moving_aabb = moving_boundary.axisAlignedBoundary();
	if (moving_aabb == nullptr) return {};

	const SpatialAabbQueryService aabb_query;
	std::vector<SpatialObjectId> overlaps;
	for (const SpatialBuildingObject &other_object : model.objects().objects()) {
		if (other_object.identity().objectId() == moving_object.identity().objectId() ||
		    other_object.identity().objectId() == permitted_target_id ||
		    !constraint_mask.contains(other_object.collisionPolicy().layer()) ||
		    !moving_object.collisionPolicy().blocks(other_object.collisionPolicy())) {
			continue;
		}
		const SpatialBoundaryModel other_boundary = derivation.deriveAtWorldTransform(
			other_object,
			model.bindingsFor(other_object.identity().objectId()),
			scene_context,
			other_object.frameState().resolvedWorldTransform(),
			primitive_instances_override);
		const AxisAlignedBoundingBoundary *other_aabb = other_boundary.axisAlignedBoundary();
		if (other_aabb == nullptr) continue;
		if (evaluated_boundary_pair_count != nullptr) {
			++*evaluated_boundary_pair_count;
		}
		if (aabb_query.overlaps(moving_aabb->bounds(), other_aabb->bounds(), tolerance)) {
			overlaps.push_back(other_object.identity().objectId());
		}
	}
	return overlaps;
}
