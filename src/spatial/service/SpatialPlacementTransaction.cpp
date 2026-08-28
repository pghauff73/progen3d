#include "spatial/service/SpatialPlacementTransaction.h"

#include "spatial/service/SpatialObjectStateTransitionService.h"
#include "spatial/service/SpatialWorldFrameResolutionService.h"

#include <cmath>

namespace {

bool matrix_is_finite(const glm::mat4 &matrix)
{
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			if (!std::isfinite(matrix[column][row])) return false;
		}
	}
	return true;
}

bool matrices_are_equal(const glm::mat4 &first,
	                    const glm::mat4 &second,
	                    float tolerance = 0.000001f)
{
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			if (std::fabs(first[column][row] - second[column][row]) > tolerance) {
				return false;
			}
		}
	}
	return true;
}

SpatialObjectState state_after_contact_resolution(SpatialObjectState current_state)
{
	if (current_state == SpatialObjectState::Validated ||
	    current_state == SpatialObjectState::Constrained ||
	    current_state == SpatialObjectState::Connected ||
	    current_state == SpatialObjectState::ContactResolved) {
		return current_state;
	}
	const SpatialObjectStateTransitionService transitions;
	SpatialObjectState state = current_state;
	const SpatialObjectState progression[] = {
		SpatialObjectState::Approximate,
		SpatialObjectState::Positioned,
		SpatialObjectState::ContactResolved};
	for (SpatialObjectState requested : progression) {
		if (state == requested) continue;
		if (transitions.canTransition(state, requested)) state = requested;
	}
	return state;
}

SpatialBuildingModel model_with_objects(
	const SpatialBuildingModel &model,
	SpatialObjectRegistry objects)
{
	return SpatialBuildingModel(
		std::move(objects),
		model.containmentTree(),
		model.connectionGraph(),
		model.constraintGraph(),
		model.geometryBindings(),
		model.resolutionRecords());
}

} // namespace

SpatialPlacementTransaction::SpatialPlacementTransaction(
	const SpatialBuildingModel &initial_model,
	const SceneGenerationContext &scene_context)
	: candidate_model_(initial_model),
	  candidate_primitive_instances_(scene_context.primitive_instances)
{
}

bool SpatialPlacementTransaction::stageConstraintResolution(
	const SpatialConstraintResolutionResult &resolution,
	std::string *diagnostic)
{
	if (!active_) {
		if (diagnostic != nullptr) *diagnostic = "Placement transaction is not active.";
		return false;
	}
	if (!resolution.succeeded() ||
	    !resolution.resolutionLocalTransform().has_value() ||
	    !resolution.finalWorldTransform().has_value() ||
	    !resolution.resolutionRecord().has_value()) {
		if (diagnostic != nullptr) {
			*diagnostic = resolution.validationReport().firstDiagnostic().empty()
				? "Cannot stage an unsuccessful spatial resolution."
				: resolution.validationReport().firstDiagnostic();
		}
		return false;
	}

	const SpatialObjectId moving_object_id =
		resolution.resolutionRecord()->sourceObjectId();
	const SpatialBuildingObject *moving_object =
		candidate_model_.objects().find(moving_object_id);
	if (moving_object == nullptr) {
		if (diagnostic != nullptr) {
			*diagnostic = "Placement transaction cannot find moving object '" +
				moving_object_id.value() + "'.";
		}
		return false;
	}

	SpatialBuildingModel staged_model = candidate_model_.replacingObject(
		moving_object->withFrameState(
			moving_object->frameState().withResolvedPlacement(
				*resolution.resolutionLocalTransform(),
				*resolution.finalWorldTransform()),
			state_after_contact_resolution(moving_object->state())));
	SpatialModelValidationReport frame_report;
	const std::optional<SpatialObjectRegistry> resolved_objects =
		SpatialWorldFrameResolutionService().resolve(
			staged_model.objects(), staged_model.containmentTree(), &frame_report);
	if (!resolved_objects.has_value()) {
		if (diagnostic != nullptr) {
			*diagnostic = frame_report.firstDiagnostic().empty()
				? "Placement transaction could not resolve descendant frames."
				: frame_report.firstDiagnostic();
		}
		return false;
	}
	staged_model = model_with_objects(staged_model, *resolved_objects);

	std::vector<ScenePrimitiveInstance> staged_instances =
		candidate_primitive_instances_;
	for (const SpatialBuildingObject &updated_object : staged_model.objects().objects()) {
		const SpatialBuildingObject *previous_object =
			candidate_model_.objects().find(updated_object.identity().objectId());
		if (previous_object == nullptr) continue;
		const glm::mat4 previous_world =
			previous_object->frameState().resolvedWorldTransform();
		const glm::mat4 updated_world =
			updated_object.frameState().resolvedWorldTransform();
		if (matrices_are_equal(previous_world, updated_world)) continue;
		if (std::fabs(glm::determinant(glm::mat3(previous_world))) <= 1.0e-8f) {
			if (diagnostic != nullptr) {
				*diagnostic = "Placement transaction encountered a non-invertible previous world frame.";
			}
			return false;
		}
		const glm::mat4 world_delta = updated_world * glm::inverse(previous_world);
		for (const SpatialObjectGeometryBinding &binding :
		     staged_model.bindingsFor(updated_object.identity().objectId())) {
			if (binding.primitiveInstanceIndex() >= staged_instances.size()) {
				if (diagnostic != nullptr) {
					*diagnostic = "Placement transaction references an unavailable primitive binding.";
				}
				return false;
			}
			ScenePrimitiveInstance &instance =
				staged_instances[binding.primitiveInstanceIndex()];
			instance.primary_transform = world_delta * instance.primary_transform;
			instance.secondary_transform = world_delta * instance.secondary_transform;
			instance.position = glm::vec3(instance.primary_transform[3]);
			instance.collision_geometry_dirty = true;
		}
	}
	staged_model = staged_model.appendingResolutionRecord(
		*resolution.resolutionRecord());
	candidate_model_ = std::move(staged_model);
	candidate_primitive_instances_ = std::move(staged_instances);
	return true;
}

bool SpatialPlacementTransaction::validate(std::string *diagnostic) const
{
	if (!active_) {
		if (diagnostic != nullptr) *diagnostic = "Placement transaction is not active.";
		return false;
	}
	for (const SpatialBuildingObject &object : candidate_model_.objects().objects()) {
		if (!matrix_is_finite(object.frameState().resolvedWorldTransform()) ||
		    !matrix_is_finite(object.frameState().resolutionLocalTransform())) {
			if (diagnostic != nullptr) {
				*diagnostic = "Placement transaction contains a non-finite object transform.";
			}
			return false;
		}
	}
	for (const SpatialObjectGeometryBinding &binding : candidate_model_.geometryBindings()) {
		if (binding.primitiveInstanceIndex() >= candidate_primitive_instances_.size()) {
			if (diagnostic != nullptr) {
				*diagnostic = "Placement transaction contains an unavailable primitive binding.";
			}
			return false;
		}
	}
	for (const ScenePrimitiveInstance &instance : candidate_primitive_instances_) {
		if (!matrix_is_finite(instance.primary_transform) ||
		    !matrix_is_finite(instance.secondary_transform)) {
			if (diagnostic != nullptr) {
				*diagnostic = "Placement transaction contains a non-finite primitive transform.";
			}
			return false;
		}
	}
	return true;
}

bool SpatialPlacementTransaction::commit(
	SpatialBuildingModel *model,
	SceneGenerationContext *scene_context,
	std::string *diagnostic)
{
	if (model == nullptr || scene_context == nullptr || !validate(diagnostic)) {
		if (diagnostic != nullptr && diagnostic->empty()) {
			*diagnostic = "Placement transaction requires valid model and scene targets.";
		}
		return false;
	}
	*model = candidate_model_;
	scene_context->primitive_instances = candidate_primitive_instances_;
	active_ = false;
	return true;
}

void SpatialPlacementTransaction::discard()
{
	active_ = false;
}
