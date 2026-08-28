#pragma once

#include "editor/model/EditorSelection.h"
#include "editor/relationship/SceneSourceAssociationIndex.h"

#include <optional>

class SceneSelectionController
{
public:
	void clear(EditorSelection &selection) const;
	void sanitize(EditorSelection &selection, int primitive_count) const;
	void armPointerSelection(EditorSelection &selection, float x, float y) const;
	bool completePointerSelection(EditorSelection &selection,
	                              float x,
	                              float y,
	                              float maximum_drag_distance) const;
	std::optional<SourceRange> selectedSourceRange(
		const EditorSelection &selection,
		const SceneSourceAssociationIndex &association_index) const;
};
