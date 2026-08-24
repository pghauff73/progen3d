#pragma once

#include <functional>
#include <string>

class GrammarEditorStatusPresentation
{
public:
	int line_number = 1;
	int column_number = 1;
	int line_count = 1;
	int zoom_percent = 100;
	int diagnostic_count = 0;
	int identifier_match_count = 0;
	bool bracket_near_cursor = false;
	bool bracket_has_pair = false;
	char bracket_character = '\0';
	std::string selected_identifier;
	bool auto_run_scheduled = false;
	float auto_run_remaining_seconds = 0.0f;
};

class GrammarEditorPanel
{
public:
	using StatusChipDrawingFunction =
		std::function<void(const std::string &text,
		                   unsigned int fill_color,
		                   unsigned int text_color,
		                   float fill_alpha,
		                   float border_alpha)>;

	void drawStatus(const GrammarEditorStatusPresentation &status,
	                const StatusChipDrawingFunction &draw_status_chip) const;
};
