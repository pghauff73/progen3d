#include "editor/controller/SceneSelectionController.h"

#include "Context.h"
#include "editor/model/GeneratedSceneSnapshot.h"
#include "geometry/service/AffineCubeFaceQuery.h"
#include "spatial/model/SpatialBuildingModel.h"

#include <algorithm>
#include <cmath>

namespace {

const ScenePrimitiveInstance *visiblePrimitive(const SceneGenerationContext *context, int index)
{
	if (context == nullptr || index < 0 ||
	    static_cast<std::size_t>(index) >= context->primitive_instances.size()) {
		return nullptr;
	}
	const ScenePrimitiveInstance &primitive = context->primitive_instances[static_cast<std::size_t>(index)];
	return primitive.removed ? nullptr : &primitive;
}

} // namespace

void SceneSelectionController::clear(EditorSelection &selection) const
{
	selection = {};
}

void SceneSelectionController::selectPrimitive(EditorSelection &selection,
                                                 const SceneGenerationContext *context,
                                                 int instance_index,
                                                 std::optional<CubeFace> face) const
{
	selection.selected_spatial_object_id.clear();
	selection.selected_face.reset();
	selection.selected_instance_index = -1;
	if (visiblePrimitive(context, instance_index) == nullptr) return;
	selection.selected_instance_index = instance_index;
	if (const auto &model = context->spatialBuildingModel()) {
		const auto &bindings = model->geometryBindings();
		const auto binding = std::find_if(bindings.begin(), bindings.end(), [instance_index](const auto &entry) {
			return entry.primitiveInstanceIndex() == static_cast<std::size_t>(instance_index);
		});
		if (binding != bindings.end()) {
			selection.selected_spatial_object_id = binding->objectId().value();
		}
	}
	if (face.has_value() && hasNamedCubeFaces(selection, context)) {
		selection.selected_face = face;
	}
}

void SceneSelectionController::selectSpatialObject(EditorSelection &selection,
                                                     const SceneGenerationContext *context,
                                                     std::string object_id) const
{
	selection.selected_spatial_object_id.clear();
	selection.selected_instance_index = -1;
	selection.selected_face.reset();
	if (context == nullptr || !context->spatialBuildingModel()) return;

	const SpatialBuildingModel &model = *context->spatialBuildingModel();
	const SpatialObjectId id(object_id);
	if (model.objects().find(id) == nullptr) return;
	selection.selected_spatial_object_id = std::move(object_id);
	const auto bindings = model.bindingsFor(id);
	if (bindings.size() == 1) {
		const int index = static_cast<int>(bindings.front().primitiveInstanceIndex());
		if (visiblePrimitive(context, index) != nullptr) selection.selected_instance_index = index;
	}
}

bool SceneSelectionController::hasNamedCubeFaces(const EditorSelection &selection,
                                                  const SceneGenerationContext *context) const
{
	const auto *primitive = visiblePrimitive(context, selection.selected_instance_index);
	if (primitive == nullptr || selection.selected_spatial_object_id.empty() ||
	    !context->spatialBuildingModel() || !hasAffineCubeFaces(*primitive)) {
		return false;
	}
	const auto bindings = context->spatialBuildingModel()->bindingsFor(
		SpatialObjectId(selection.selected_spatial_object_id));
	return bindings.size() == 1 &&
	       bindings.front().primitiveInstanceIndex() == static_cast<std::size_t>(selection.selected_instance_index);
}

void SceneSelectionController::rebindToScene(EditorSelection &selection,
                                              const SceneGenerationContext *context,
                                              bool keep_ordinal_selection) const
{
	const int previous_index = selection.selected_instance_index;
	const auto previous_face = selection.selected_face;
	std::string object_id = std::move(selection.selected_spatial_object_id);
	if (keep_ordinal_selection) {
		// A timeline sample can arrive between press and release.
		selection.hovered_instance_index = -1;
		selection.hovered_face.reset();
	} else {
		clear(selection);
	}
	if (!object_id.empty()) {
		selectSpatialObject(selection, context, std::move(object_id));
		if (previous_face.has_value() && hasNamedCubeFaces(selection, context)) {
			selection.selected_face = previous_face;
		}
	} else if (keep_ordinal_selection) {
		selectPrimitive(selection, context, previous_index);
	}
}

void SceneSelectionController::armPointerSelection(EditorSelection &selection,
	                                                float x,
	                                                float y) const
{
	selection.pointer_selection_armed = true;
	selection.pointer_selection_start.x = x;
	selection.pointer_selection_start.y = y;
}

bool SceneSelectionController::completePointerSelection(
	EditorSelection &selection,
	float x,
	float y,
	float maximum_drag_distance) const
{
	if (!selection.pointer_selection_armed) {
		return false;
	}
	selection.pointer_selection_armed = false;
	const float delta_x = x - selection.pointer_selection_start.x;
	const float delta_y = y - selection.pointer_selection_start.y;
	if (std::sqrt(delta_x * delta_x + delta_y * delta_y) > maximum_drag_distance) {
		return false;
	}
	selection.selected_instance_index = selection.hovered_instance_index;
	return true;
}

std::optional<SourceRange> SceneSelectionController::selectedSourceRange(
	const EditorSelection &selection,
	const SceneSourceAssociationIndex &association_index,
	const GeneratedSceneSnapshot *snapshot,
	std::string_view editor_source) const
{
	if (snapshot == nullptr || snapshot->sourceText() != editor_source) return std::nullopt;
	return association_index.sourceRangeForInstance(selection.selected_instance_index);
}
