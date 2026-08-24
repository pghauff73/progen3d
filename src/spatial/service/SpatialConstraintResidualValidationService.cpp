#include "spatial/service/SpatialConstraintResidualValidationService.h"

#include "spatial/service/SpatialAabbQueryService.h"
#include "spatial/service/SpatialCollisionQueryService.h"
#include "spatial/service/SpatialDistanceQueryService.h"
#include "spatial/service/SpatialInterfaceQueryService.h"
#include "spatial/service/SpatialObjectBoundaryDerivationService.h"

#include <cmath>

SpatialModelValidationReport SpatialConstraintResidualValidationService::validate(
	const CollisionPositionConstraint &constraint,
	const SpatialBuildingModel &model,
	const SceneGenerationContext &scene_context,
	const std::vector<ScenePrimitiveInstance> *primitive_instances_override,
	const SpatialPositioningSafetyLimits &limits) const
{
	SpatialModelValidationReport report;
	const SpatialBuildingObject *moving = model.objects().find(constraint.movingObjectId());
	const SpatialBuildingObject *target = model.objects().find(constraint.targetObjectId());
	if (moving == nullptr || target == nullptr) {
		report.addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::UndefinedConstraintEndpoint,
			"Residual validation cannot resolve constraint endpoints.",
			{constraint.movingObjectId(), constraint.targetObjectId()}));
		return report;
	}
	const SpatialInterface *moving_interface =
		moving->findInterface(constraint.movingInterfaceId());
	const SpatialInterface *target_interface =
		target->findInterface(constraint.targetInterfaceId());
	if (moving_interface == nullptr || target_interface == nullptr) {
		report.addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::UndefinedConstraintEndpoint,
			"Residual validation cannot resolve constraint interfaces.",
			{constraint.movingObjectId(), constraint.targetObjectId()}));
		return report;
	}

	const SpatialObjectBoundaryDerivationService derivation;
	const SpatialBoundaryModel moving_boundary = derivation.deriveAtWorldTransform(
		*moving, model.bindingsFor(moving->identity().objectId()), scene_context,
		moving->frameState().resolvedWorldTransform(), primitive_instances_override);
	const SpatialBoundaryModel target_boundary = derivation.deriveAtWorldTransform(
		*target, model.bindingsFor(target->identity().objectId()), scene_context,
		target->frameState().resolvedWorldTransform(), primitive_instances_override);
	const AxisAlignedBoundingBoundary *moving_aabb = moving_boundary.axisAlignedBoundary();
	const AxisAlignedBoundingBoundary *target_aabb = target_boundary.axisAlignedBoundary();
	if (moving_aabb == nullptr || target_aabb == nullptr) {
		report.addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::SpatialQueryFailed,
			"Residual validation requires object boundaries.",
			{constraint.movingObjectId(), constraint.targetObjectId()}));
		return report;
	}
	const SpatialRelationResult relation = SpatialDistanceQueryService().measure(
		moving->identity().objectId(), moving_aabb->bounds(),
		target->identity().objectId(), target_aabb->bounds(), constraint.tolerance());
	const float required_clearance = constraint.mode() == CollisionPositionMode::Gap
		? constraint.clearance().nominal()
		: 0.0f;
	if (relation.penetration() > limits.contact_slop ||
	    std::fabs(relation.distance() - required_clearance) > constraint.tolerance() ||
	    !constraint.clearance().accepts(relation.distance(), constraint.tolerance())) {
		report.addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::ResidualConstraintViolation,
			"Constraint '" + constraint.constraintId().value() +
				" no longer satisfies its clearance residual.",
			{constraint.movingObjectId(), constraint.targetObjectId()}));
		return report;
	}

	const SpatialInterfaceQueryService interface_query;
	const SpatialInterfaceProjection moving_projection = interface_query.project(
		*moving, *moving_interface, moving->frameState().resolvedWorldTransform(),
		moving_aabb->bounds());
	const SpatialInterfaceProjection target_projection = interface_query.project(
		*target, *target_interface, target->frameState().resolvedWorldTransform(),
		target_aabb->bounds());
	if (!SpatialAabbQueryService().touchesOrOverlaps(
			moving_projection.worldRegionBounds(), target_projection.worldRegionBounds(),
			required_clearance + constraint.tolerance())) {
		report.addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::ResidualConstraintViolation,
			"Constraint '" + constraint.constraintId().value() +
				" no longer has overlapping interface regions.",
			{constraint.movingObjectId(), constraint.targetObjectId()}));
		return report;
	}

	const std::vector<SpatialObjectId> forbidden =
		SpatialCollisionQueryService().findForbiddenOverlaps(
			*moving, moving->frameState().resolvedWorldTransform(),
			target->identity().objectId(), constraint.collisionMask(), model,
			scene_context, primitive_instances_override, limits.contact_slop);
	if (!forbidden.empty()) {
		report.addIssue(SpatialModelValidationIssue(
			SpatialModelValidationCode::ForbiddenCollision,
			"Constraint '" + constraint.constraintId().value() +
				" leaves a forbidden overlap with object '" +
				forbidden.front().value() + "'.",
			{constraint.movingObjectId(), forbidden.front()}));
	}
	return report;
}
