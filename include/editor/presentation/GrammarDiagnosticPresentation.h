#pragma once

#include "editor/model/DocumentDiagnosticCollection.h"
#include "editor/model/GrammarEditorInteractionState.h"

#include <vector>

class GrammarDiagnosticPresentation
{
public:
	std::vector<EditorDiagnostic> buildDiagnostics(
		const std::vector<std::string> &source_lines,
		const DocumentDiagnosticCollection *compiler_diagnostics = nullptr) const;
};
