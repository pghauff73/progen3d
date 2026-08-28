#include "editor/presentation/DiagnosticsPanel.h"

#include <imgui.h>

#include <string>

void DiagnosticsPanel::draw(
	const std::vector<EditorDiagnostic> &diagnostics,
	const SourceNavigationFunction &navigate_to_source)
{
	ImGui::TextUnformatted("Diagnostics");
	ImGui::SameLine();
	ImGui::TextDisabled("%zu issues", diagnostics.size());
	ImGui::Checkbox("Errors", &show_errors_);
	ImGui::SameLine();
	ImGui::Checkbox("Warnings", &show_warnings_);
	ImGui::Separator();

	for (std::size_t index = 0; index < diagnostics.size(); ++index) {
		const EditorDiagnostic &diagnostic = diagnostics[index];
		const bool is_error = diagnostic.severity == EditorDiagnosticSeverity::Error;
		if ((is_error && !show_errors_) || (!is_error && !show_warnings_)) {
			continue;
		}
		ImGui::PushID(static_cast<int>(index));
		const std::string location = diagnostic.range.found
			                             ? "Line " + std::to_string(diagnostic.range.line + 1) +
			                                   ", Column " +
			                                   std::to_string(diagnostic.range.start_column + 1)
			                             : "No source location";
		if (ImGui::Selectable(location.c_str(), false) && navigate_to_source &&
		    diagnostic.range.found) {
			navigate_to_source(diagnostic.range);
		}
		ImGui::SameLine();
		ImGui::TextDisabled("%s", is_error ? "Error" : "Warning");
		ImGui::TextWrapped("%s", diagnostic.message.c_str());
		ImGui::Separator();
		ImGui::PopID();
	}
}
