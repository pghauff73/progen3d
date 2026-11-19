#pragma once

#include "editor/model/EditorSelection.h"
#include "editor/relationship/SceneSourceAssociationIndex.h"

#include <optional>
#include <string_view>

class GeneratedSceneSnapshot;
class SceneGenerationContext;

class SceneSelectionController
{
public:
	void clear(EditorSelection &selection) const;
	void selectPrimitive(EditorSelection &selection,
	                     const SceneGenerationContext *context,
	                     int instance_index,
	                     std::optional<CubeFace> face = std::nullopt) const;
	void selectSpatialObject(EditorSelection &selection,
	                         const SceneGenerationContext *context,
	                         std::string object_id) const;
	bool hasNamedCubeFaces(const EditorSelection &selection,
	                       const SceneGenerationContext *context) const;
	void rebindToScene(EditorSelection &selection,
	                   const SceneGenerationContext *context,
	                   bool keep_ordinal_selection) const;
	void armPointerSelection(EditorSelection &selection, float x, float y) const;
	bool completePointerSelection(EditorSelection &selection,
	                              float x,
	                              float y,
	                              float maximum_drag_distance) const;
	std::optional<SourceRange> selectedSourceRange(
		const EditorSelection &selection,
		const SceneSourceAssociationIndex &association_index,
		const GeneratedSceneSnapshot *snapshot,
		std::string_view editor_source) const;
};
