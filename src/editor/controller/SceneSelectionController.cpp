#include "editor/controller/SceneSelectionController.h"

#include <cmath>

void SceneSelectionController::clear(EditorSelection &selection) const
{
	selection.hovered_instance_index = -1;
	selection.selected_instance_index = -1;
	selection.pointer_selection_armed = false;
}

void SceneSelectionController::sanitize(EditorSelection &selection, int primitive_count) const
{
	if (selection.hovered_instance_index < 0 ||
	    selection.hovered_instance_index >= primitive_count) {
		selection.hovered_instance_index = -1;
	}
	if (selection.selected_instance_index < 0 ||
	    selection.selected_instance_index >= primitive_count) {
		selection.selected_instance_index = -1;
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
	const SceneSourceAssociationIndex &association_index) const
{
	return association_index.sourceRangeForInstance(selection.selected_instance_index);
}
