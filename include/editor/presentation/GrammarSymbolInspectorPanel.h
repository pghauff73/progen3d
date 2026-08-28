#pragma once

#include "editor/model/GrammarEditorInteractionState.h"

#include <functional>
#include <string>
#include <vector>

class GrammarSymbolInspection
{
public:
	EditorToken selected_token;
	int selected_line = -1;
	int selected_column = -1;
	std::vector<EditorRange> occurrence_ranges;
	int active_occurrence_index = -1;
};

class GrammarSymbolInspectorPanel
{
public:
	using DetailDrawingFunction = std::function<bool(const EditorToken &selected_token)>;
	using SourceNavigationFunction = std::function<void(const EditorRange &source_range)>;

	void drawPopup(const GrammarSymbolInspection *inspection,
	               const DetailDrawingFunction &draw_details,
	               const SourceNavigationFunction &navigate_to_source) const;

private:
	std::string labelForTokenKind(EditorTokenKind token_kind) const;
};
