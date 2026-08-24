#include "editor/presentation/GrammarEditorPanel.h"

#include <imgui.h>

#include <cstdio>

namespace {

void draw_next_status_chip(const GrammarEditorPanel::StatusChipDrawingFunction &draw_status_chip,
	                       const std::string &text,
	                       unsigned int fill_color,
	                       unsigned int text_color,
	                       float fill_alpha = 0.16f,
	                       float border_alpha = 0.32f)
{
	ImGui::SameLine();
	draw_status_chip(text, fill_color, text_color, fill_alpha, border_alpha);
}

}

void GrammarEditorPanel::drawStatus(
	const GrammarEditorStatusPresentation &status,
	const StatusChipDrawingFunction &draw_status_chip) const
{
	if (!draw_status_chip) {
		return;
	}

	char position_label[64];
	std::snprintf(position_label,
	              sizeof(position_label),
	              "Ln %d  Col %d",
	              status.line_number,
	              status.column_number);
	draw_status_chip(position_label, 0x8ea5ba, 0x556d83, 0.16f, 0.32f);
	draw_next_status_chip(draw_status_chip,
	                      std::to_string(status.line_count) + " lines",
	                      0x8ea5ba,
	                      0x556d83);
	draw_next_status_chip(draw_status_chip,
	                      "Zoom " + std::to_string(status.zoom_percent) + "%",
	                      0x8ea5ba,
	                      0x556d83);

	if (status.diagnostic_count > 0) {
		draw_next_status_chip(draw_status_chip,
		                      std::to_string(status.diagnostic_count) + " issue" +
			                      (status.diagnostic_count == 1 ? "" : "s"),
		                      0xca7f7f,
		                      0x7f4848,
		                      0.12f,
		                      0.24f);
	}
	if (status.identifier_match_count > 0) {
		draw_next_status_chip(draw_status_chip,
		                      "Matches " + std::to_string(status.identifier_match_count),
		                      0x7b93ae,
		                      0x516a83);
	}
	if (status.bracket_near_cursor) {
		if (status.bracket_has_pair) {
			draw_next_status_chip(draw_status_chip,
			                      "Bracket matched",
			                      0x7ea88e,
			                      0x527564);
		} else {
			draw_next_status_chip(draw_status_chip,
			                      std::string("Unmatched ") + status.bracket_character,
			                      0xca7f7f,
			                      0x7f4848,
			                      0.12f,
			                      0.24f);
		}
	}
	if (!status.selected_identifier.empty()) {
		draw_next_status_chip(draw_status_chip,
		                      "Selected " + status.selected_identifier,
		                      0x6f95ba,
		                      0x436786);
	}
	if (status.auto_run_scheduled) {
		char auto_run_label[48];
		std::snprintf(auto_run_label,
		              sizeof(auto_run_label),
		              "Auto-run %.1fs",
		              status.auto_run_remaining_seconds);
		draw_next_status_chip(draw_status_chip,
		                      auto_run_label,
		                      0xc8a766,
		                      0x7a6035,
		                      0.14f,
		                      0.26f);
	}
}
