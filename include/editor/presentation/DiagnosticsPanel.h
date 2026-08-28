#pragma once

#include "editor/model/GrammarEditorInteractionState.h"

#include <functional>
#include <vector>

class DiagnosticsPanel
{
public:
	using SourceNavigationFunction = std::function<void(const EditorRange &source_range)>;

	void draw(const std::vector<EditorDiagnostic> &diagnostics,
	          const SourceNavigationFunction &navigate_to_source);

private:
	bool show_errors_ = true;
	bool show_warnings_ = true;
};
