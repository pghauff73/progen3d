#include "spatial/service/SpatialObjectBoundaryDerivationService.h"

#include "Context.h"
#include "spatial/model/AxisAlignedBoundingBoundary.h"
#include "spatial/service/SpatialAabbQueryService.h"
#include "spatial/service/SpatialInterfaceQueryService.h"

#include <cmath>
#include <memory>

namespace {

bool matrix_is_invertible(const glm::mat4 &matrix)
{
	return std::fabs(glm::determinant(glm::mat3(matrix))) > 1.0e-8f;
}

} // namespace

SpatialBoundaryModel SpatialObjectBoundaryDerivationService::derive(
	const SpatialBuildingObject &object,
	const std::vector<SpatialObjectGeometryBinding> &bindings,
	const SceneGenerationContext &scene_context) const
{
	return deriveAtWorldTransform(
		object,
		bindings,
		scene_context,
		object.frameState().resolvedWorldTransform());
}

SpatialBoundaryModel SpatialObjectBoundaryDerivationService::deriveAtWorldTransform(
	const SpatialBuildingObject &object,
	const std::vector<SpatialObjectGeometryBinding> &bindings,
	const SceneGenerationContext &scene_context,
	const glm::mat4 &candidate_world_transform,
	const std::vector<ScenePrimitiveInstance> *primitive_instances_override) const
{
	const SpatialAabbQueryService aabb_query;
	AxisAlignedBounds aggregate;
	const AxisAlignedBoundingBoundary *explicit_boundary =
		object.boundaryModel().axisAlignedBoundary();
	if (explicit_boundary != nullptr) {
		aggregate = aabb_query.transform(
			explicit_boundary->bounds(), candidate_world_transform);
	}

	const std::vector<ScenePrimitiveInstance> &primitive_instances =
		primitive_instances_override != nullptr
			? *primitive_instances_override
			: scene_context.primitive_instances;
	const glm::mat4 current_world = object.frameState().resolvedWorldTransform();
	if (matrix_is_invertible(current_world)) {
		const glm::mat4 world_delta = candidate_world_transform * glm::inverse(current_world);
		for (const SpatialObjectGeometryBinding &binding : bindings) {
			if (binding.objectId() != object.identity().objectId() ||
			    binding.primitiveInstanceIndex() >= primitive_instances.size()) {
				continue;
			}
			ScenePrimitiveInstance candidate =
				primitive_instances[binding.primitiveInstanceIndex()];
			if (candidate.removed) continue;
			candidate.primary_transform = world_delta * candidate.primary_transform;
			candidate.secondary_transform = world_delta * candidate.secondary_transform;
			candidate.position = glm::vec3(candidate.primary_transform[3]);
			candidate.collision_geometry_dirty = true;
			aggregate = aabb_query.merged(
				aggregate, scene_context.getInstanceBounds(candidate));
		}
	}

	if (!aggregate.valid) {
		const SpatialInterfaceQueryService interface_query;
		for (const SpatialInterface &interface : object.interfaces()) {
			const SpatialInterfaceProjection projection = interface_query.project(
				object, interface, candidate_world_transform);
			aggregate = aabb_query.merged(
				aggregate, projection.worldRegionBounds());
		}
	}

	if (!aggregate.valid) return {};
	return SpatialBoundaryModel({
		std::make_shared<const AxisAlignedBoundingBoundary>(aggregate)});
}
