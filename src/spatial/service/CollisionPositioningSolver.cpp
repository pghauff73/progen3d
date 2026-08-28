#include "spatial/service/CollisionPositioningSolver.h"

#include "spatial/model/SpatialInterfaceCompatibilityResult.h"
#include "spatial/service/SpatialAabbQueryService.h"
#include "spatial/service/SpatialCollisionQueryService.h"
#include "spatial/service/SpatialDirectionResolutionService.h"
#include "spatial/service/SpatialDistanceQueryService.h"
#include "spatial/service/SpatialInterfaceCompatibilityService.h"
#include "spatial/service/SpatialInterfaceQueryService.h"
#include "spatial/service/SpatialObjectBoundaryDerivationService.h"
#include "spatial/service/SpatialResolutionEvidenceHashService.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace {

constexpr float kDirectionTolerance = 1.0e-7f;

bool matrix_is_finite(const glm::mat4 &matrix)
{
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			if (!std::isfinite(matrix[column][row])) return false;
		}
	}
	return true;
}

struct SweepInterval {
	float entry = 0.0f;
	float exit = 0.0f;
	bool valid = false;
};

SweepInterval calculate_sweep_interval(const AxisAlignedBounds &moving,
	                                   const AxisAlignedBounds &target,
	                                   const glm::vec3 &direction,
	                                   float maximum_distance)
{
	float entry = -std::numeric_limits<float>::infinity();
	float exit = std::numeric_limits<float>::infinity();
	for (int axis = 0; axis < 3; ++axis) {
		if (std::fabs(direction[axis]) <= kDirectionTolerance) {
			if (moving.max[axis] < target.min[axis] ||
			    target.max[axis] < moving.min[axis]) {
				return {};
			}
			continue;
		}
		const float first_time =
			(target.min[axis] - moving.max[axis]) / direction[axis];
		const float second_time =
			(target.max[axis] - moving.min[axis]) / direction[axis];
		entry = std::max(entry, std::min(first_time, second_time));
		exit = std::min(exit, std::max(first_time, second_time));
	}
	if (exit < 0.0f || entry > exit || entry > maximum_distance) return {};
	SweepInterval interval;
	interval.entry = std::max(0.0f, entry);
	interval.exit = std::min(maximum_distance, exit);
	interval.valid = interval.entry <= interval.exit;
	return interval;
}

void add_failure(SpatialModelValidationReport *report,
	             SpatialModelValidationCode code,
	             const std::string &message,
	             const CollisionPositionConstraint &constraint)
{
	report->addIssue(SpatialModelValidationIssue(
		code,
		"Spatial constraint '" + constraint.constraintId().value() + "': " + message,
		{constraint.movingObjectId(), constraint.targetObjectId()}));
}

glm::mat4 parent_world_transform(const SpatialBuildingObject &moving_object,
	                             const SpatialBuildingModel &model)
{
	const SpatialObjectId *parent_id =
		model.containmentTree().parentOf(moving_object.identity().objectId());
	if (parent_id == nullptr) return glm::mat4(1.0f);
	const SpatialBuildingObject *parent = model.objects().find(*parent_id);
	return parent != nullptr
		? parent->frameState().resolvedWorldTransform()
		: glm::mat4(1.0f);
}

} // namespace

SpatialConstraintResolutionResult CollisionPositioningSolver::solve(
	const CollisionPositionConstraint &constraint,
	const SpatialBuildingModel &model,
	const SceneGenerationContext &scene_context,
	const std::vector<ScenePrimitiveInstance> *primitive_instances_override,
	const SpatialPositioningSafetyLimits &limits) const
{
	SpatialModelValidationReport report;
	const SpatialBuildingObject *moving_object =
		model.objects().find(constraint.movingObjectId());
	const SpatialBuildingObject *target_object =
		model.objects().find(constraint.targetObjectId());
	if (moving_object == nullptr || target_object == nullptr || moving_object == target_object) {
		add_failure(&report, SpatialModelValidationCode::UndefinedConstraintEndpoint,
		            "moving and target objects must be distinct and defined.", constraint);
		return SpatialConstraintResolutionResult::failed(std::move(report));
	}
	const SpatialInterface *moving_interface =
		moving_object->findInterface(constraint.movingInterfaceId());
	const SpatialInterface *target_interface =
		target_object->findInterface(constraint.targetInterfaceId());
	if (moving_interface == nullptr || target_interface == nullptr) {
		add_failure(&report, SpatialModelValidationCode::UndefinedConstraintEndpoint,
		            "moving and target interfaces must be defined.", constraint);
		return SpatialConstraintResolutionResult::failed(std::move(report));
	}
	const SpatialInterfaceCompatibilityResult compatibility =
		SpatialInterfaceCompatibilityService().evaluate(
			*moving_interface, *target_interface, constraint.tolerance());
	if (!compatibility.isCompatible()) {
		add_failure(&report, SpatialModelValidationCode::IncompatibleInterfaces,
		            compatibility.diagnostic(), constraint);
		return SpatialConstraintResolutionResult::failed(std::move(report));
	}
	if (constraint.mode() != CollisionPositionMode::Touch &&
	    constraint.mode() != CollisionPositionMode::Gap &&
	    constraint.mode() != CollisionPositionMode::Drop) {
		add_failure(&report, SpatialModelValidationCode::UnsupportedConstraint,
		            "the requested positioning mode is not implemented by SMB-OMv2 P0.",
		            constraint);
		return SpatialConstraintResolutionResult::failed(std::move(report));
	}
	if (!std::isfinite(constraint.maximumDistance()) ||
	    constraint.maximumDistance() <= 0.0f ||
	    !std::isfinite(constraint.tolerance()) ||
	    constraint.tolerance() < limits.minimum_tolerance ||
	    !std::isfinite(constraint.clearance().nominal()) ||
	    constraint.clearance().nominal() < 0.0f) {
		add_failure(&report, SpatialModelValidationCode::InvalidConstraint,
		            "travel, tolerance, and clearance values must be finite and supported.",
		            constraint);
		return SpatialConstraintResolutionResult::failed(std::move(report));
	}
	const SpatialCollisionQueryService collision_query;
	if (!collision_query.targetContactIsPermitted(
			*moving_object, *target_object, constraint.collisionMask())) {
		add_failure(&report, SpatialModelValidationCode::InvalidConstraint,
		            "collision layers or masks do not permit contact with the target.",
		            constraint);
		return SpatialConstraintResolutionResult::failed(std::move(report));
	}

	std::string direction_diagnostic;
	const std::optional<glm::vec3> direction =
		SpatialDirectionResolutionService().resolve(
			constraint.direction(), *moving_object, *moving_interface,
			*target_object, *target_interface, model, &direction_diagnostic);
	if (!direction.has_value()) {
		add_failure(&report, SpatialModelValidationCode::InvalidConstraint,
		            direction_diagnostic, constraint);
		return SpatialConstraintResolutionResult::failed(std::move(report));
	}

	const SpatialObjectBoundaryDerivationService derivation;
	const SpatialBoundaryModel moving_boundary = derivation.deriveAtWorldTransform(
		*moving_object,
		model.bindingsFor(moving_object->identity().objectId()),
		scene_context,
		moving_object->frameState().resolvedWorldTransform(),
		primitive_instances_override);
	const SpatialBoundaryModel target_boundary = derivation.deriveAtWorldTransform(
		*target_object,
		model.bindingsFor(target_object->identity().objectId()),
		scene_context,
		target_object->frameState().resolvedWorldTransform(),
		primitive_instances_override);
	const AxisAlignedBoundingBoundary *moving_aabb = moving_boundary.axisAlignedBoundary();
	const AxisAlignedBoundingBoundary *target_aabb = target_boundary.axisAlignedBoundary();
	if (moving_aabb == nullptr || target_aabb == nullptr) {
		add_failure(&report, SpatialModelValidationCode::SpatialQueryFailed,
		            "moving and target objects require positionable boundaries.", constraint);
		return SpatialConstraintResolutionResult::failed(std::move(report));
	}

	const SpatialAabbQueryService aabb_query;
	int collision_query_count = 1;
	if (aabb_query.overlaps(
			moving_aabb->bounds(), target_aabb->bounds(), limits.contact_slop)) {
		add_failure(&report, SpatialModelValidationCode::PositioningFailed,
		            "starting overlap is not supported by the P0 translation solver.",
		            constraint);
		return SpatialConstraintResolutionResult::failed(std::move(report));
	}

	const SweepInterval interval = calculate_sweep_interval(
		moving_aabb->bounds(), target_aabb->bounds(), *direction,
		constraint.maximumDistance());
	if (!interval.valid) {
		add_failure(&report, SpatialModelValidationCode::PositioningFailed,
		            "the target was not reached within maximum travel.", constraint);
		return SpatialConstraintResolutionResult::failed(std::move(report));
	}

	float separated_distance = interval.entry;
	float intersecting_distance = interval.entry;
	int refinement_iterations = 0;
	const float available_overlap_distance = interval.exit - interval.entry;
	if (available_overlap_distance > limits.minimum_tolerance) {
		intersecting_distance = interval.entry +
			std::min(available_overlap_distance * 0.5f,
			         std::max(constraint.tolerance() * 4.0f,
			                  limits.minimum_tolerance * 4.0f));
		for (; refinement_iterations < limits.maximum_refinement_iterations &&
		       intersecting_distance - separated_distance > constraint.tolerance();
		     ++refinement_iterations) {
			const float midpoint =
				(separated_distance + intersecting_distance) * 0.5f;
			const AxisAlignedBounds candidate = aabb_query.translated(
				moving_aabb->bounds(), *direction * midpoint);
			++collision_query_count;
			if (aabb_query.overlaps(candidate, target_aabb->bounds())) {
				intersecting_distance = midpoint;
			} else {
				separated_distance = midpoint;
			}
		}
	}

	const float requested_clearance = constraint.mode() == CollisionPositionMode::Gap
		? constraint.clearance().nominal()
		: 0.0f;
	const float final_distance =
		std::max(0.0f, separated_distance - requested_clearance);
	const glm::mat4 initial_world = moving_object->frameState().resolvedWorldTransform();
	const glm::mat4 final_world =
		glm::translate(glm::mat4(1.0f), *direction * final_distance) * initial_world;
	if (!matrix_is_finite(final_world)) {
		add_failure(&report, SpatialModelValidationCode::PositioningFailed,
		            "candidate world transform is non-finite.", constraint);
		return SpatialConstraintResolutionResult::failed(std::move(report));
	}

	const SpatialBoundaryModel final_boundary = derivation.deriveAtWorldTransform(
		*moving_object,
		model.bindingsFor(moving_object->identity().objectId()),
		scene_context,
		final_world,
		primitive_instances_override);
	const AxisAlignedBoundingBoundary *final_aabb = final_boundary.axisAlignedBoundary();
	if (final_aabb == nullptr) {
		add_failure(&report, SpatialModelValidationCode::SpatialQueryFailed,
		            "candidate boundary derivation failed.", constraint);
		return SpatialConstraintResolutionResult::failed(std::move(report));
	}
	const SpatialRelationResult final_relation = SpatialDistanceQueryService().measure(
		moving_object->identity().objectId(), final_aabb->bounds(),
		target_object->identity().objectId(), target_aabb->bounds(),
		constraint.tolerance());
	++collision_query_count;
	const float resulting_clearance = final_relation.distance();
	const float residual_error =
		std::fabs(resulting_clearance - requested_clearance);
	if (residual_error > constraint.tolerance() ||
	    !constraint.clearance().accepts(
		    resulting_clearance, constraint.tolerance())) {
		add_failure(&report, SpatialModelValidationCode::ResidualConstraintViolation,
		            "resulting clearance is outside the requested tolerance.", constraint);
		return SpatialConstraintResolutionResult::failed(std::move(report));
	}

	const SpatialInterfaceQueryService interface_query;
	const SpatialInterfaceProjection moving_projection = interface_query.project(
		*moving_object, *moving_interface, final_world, final_aabb->bounds());
	const SpatialInterfaceProjection target_projection = interface_query.project(
		*target_object, *target_interface,
		target_object->frameState().resolvedWorldTransform(), target_aabb->bounds());
	if (!aabb_query.touchesOrOverlaps(
			moving_projection.worldRegionBounds(),
			target_projection.worldRegionBounds(),
			requested_clearance + constraint.tolerance())) {
		add_failure(&report, SpatialModelValidationCode::ResidualConstraintViolation,
		            "moving and target interface regions do not overlap at the resolved pose.",
		            constraint);
		return SpatialConstraintResolutionResult::failed(std::move(report));
	}
	++collision_query_count;

	int forbidden_boundary_pair_count = 0;
	const std::vector<SpatialObjectId> forbidden =
		collision_query.findForbiddenOverlaps(
			*moving_object, final_world, target_object->identity().objectId(),
			constraint.collisionMask(), model, scene_context,
			primitive_instances_override, limits.contact_slop,
			&forbidden_boundary_pair_count);
	collision_query_count += forbidden_boundary_pair_count;
	if (!forbidden.empty()) {
		add_failure(&report, SpatialModelValidationCode::ForbiddenCollision,
		            "candidate pose overlaps forbidden object '" +
				forbidden.front().value() + "'.",
		            constraint);
		return SpatialConstraintResolutionResult::failed(std::move(report));
	}

	const glm::mat4 authored_world =
		parent_world_transform(*moving_object, model) *
		moving_object->frameState().authoredLocalTransform();
	if (std::fabs(glm::determinant(glm::mat3(authored_world))) <= kDirectionTolerance) {
		add_failure(&report, SpatialModelValidationCode::InvalidFrame,
		            "authored object frame is not invertible.", constraint);
		return SpatialConstraintResolutionResult::failed(std::move(report));
	}
	const glm::mat4 resolution_local = glm::inverse(authored_world) * final_world;
	SpatialResolutionRecord record(
		constraint.constraintId(), moving_object->identity().objectId(),
		target_object->identity().objectId(), initial_world, final_world,
		SpatialResolutionAlgorithm::AxisAlignedSweep, 1, refinement_iterations,
		collision_query_count, constraint.tolerance(), final_relation.contactPoint(), -*direction,
		resulting_clearance, residual_error, SpatialResolutionStatus::Succeeded,
		{"P0 positioning used axis-aligned bounding boundaries."}, 0u);
	record = SpatialResolutionEvidenceHashService().attachCalculatedHash(record);
	return SpatialConstraintResolutionResult::succeeded(
		resolution_local, final_world, std::move(record));
}
