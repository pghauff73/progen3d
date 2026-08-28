#include "editor/presentation/GrammarSymbolInspectorPanel.h"

#include <imgui.h>

std::string GrammarSymbolInspectorPanel::labelForTokenKind(EditorTokenKind token_kind) const
{
	switch (token_kind) {
	case EditorTokenKind::RuleDefinition:
		return "Rule Definition";
	case EditorTokenKind::RuleReference:
		return "Rule Reference";
	case EditorTokenKind::Variable:
		return "Variable";
	case EditorTokenKind::Primitive:
		return "Primitive";
	case EditorTokenKind::Identifier:
		return "Identifier";
	case EditorTokenKind::Keyword:
		return "Keyword";
	case EditorTokenKind::Operator:
		return "Operator";
	case EditorTokenKind::Number:
		return "Number";
	case EditorTokenKind::Comment:
		return "Comment";
	case EditorTokenKind::Plain:
	default:
		return "Text";
	}
}

void GrammarSymbolInspectorPanel::drawPopup(
	const GrammarSymbolInspection *inspection,
	const DetailDrawingFunction &draw_details,
	const SourceNavigationFunction &navigate_to_source) const
{
	if (!ImGui::BeginPopup("IdentifierSelectionTooltip")) {
		return;
	}
	if (inspection == nullptr) {
		ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
		return;
	}

	const EditorToken &selected_token = inspection->selected_token;
	ImGui::TextUnformatted(selected_token.text.c_str());
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.61f, 0.58f, 0.55f, 1.0f));
	ImGui::TextUnformatted(labelForTokenKind(selected_token.kind).c_str());
	ImGui::PopStyleColor();
	ImGui::Separator();
	ImGui::Text("Line %d, Column %d",
	            inspection->selected_line + 1,
	            inspection->selected_column + 1);
	ImGui::Text("Occurrences: %zu", inspection->occurrence_ranges.size());
	if (draw_details && draw_details(selected_token)) {
		ImGui::Separator();
	}
	ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
	ImGui::TextWrapped("%s", selected_token.tooltip.c_str());
	ImGui::PopTextWrapPos();

	if (!inspection->occurrence_ranges.empty()) {
		ImGui::Separator();
		if (inspection->active_occurrence_index >= 0) {
			ImGui::Text("Instance %d of %zu",
			            inspection->active_occurrence_index + 1,
			            inspection->occurrence_ranges.size());
		} else {
			ImGui::Text("Instances: %zu", inspection->occurrence_ranges.size());
		}

		const bool has_previous_occurrence = inspection->active_occurrence_index > 0;
		const bool has_next_occurrence =
			inspection->active_occurrence_index >= 0 &&
			inspection->active_occurrence_index + 1 <
				static_cast<int>(inspection->occurrence_ranges.size());
		if (has_previous_occurrence) {
			if (ImGui::ArrowButton("##prev_identifier_occurrence", ImGuiDir_Left) &&
			    navigate_to_source) {
				navigate_to_source(inspection->occurrence_ranges[
					static_cast<std::size_t>(inspection->active_occurrence_index - 1)]);
			}
			ImGui::SameLine();
			ImGui::TextUnformatted("Previous");
		}
		if (has_previous_occurrence && has_next_occurrence) {
			ImGui::SameLine();
			ImGui::TextUnformatted("|");
			ImGui::SameLine();
		}
		if (has_next_occurrence) {
			if (ImGui::ArrowButton("##next_identifier_occurrence", ImGuiDir_Right) &&
			    navigate_to_source) {
				navigate_to_source(inspection->occurrence_ranges[
					static_cast<std::size_t>(inspection->active_occurrence_index + 1)]);
			}
			ImGui::SameLine();
			ImGui::TextUnformatted("Next");
		}
	}

	ImGui::EndPopup();
}
