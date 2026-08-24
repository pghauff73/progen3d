#pragma once

#include <string>
#include <vector>

enum class EditorTokenKind {
	Plain,
	Comment,
	RuleDefinition,
	RuleReference,
	Variable,
	Number,
	Operator,
	Keyword,
	Primitive,
	Identifier
};

struct EditorToken {
	std::string text;
	EditorTokenKind kind = EditorTokenKind::Plain;
	std::string tooltip;
};

struct EditorTokenSpan {
	EditorToken token;
	int start_column = 0;
	int end_column = 0;
};

struct EditorTokenMatch {
	int line = -1;
	int column = 0;
	bool found = false;
};

struct EditorRange {
	int line = -1;
	int start_column = 0;
	int end_column = 0;
	bool found = false;
};

enum class EditorDiagnosticSeverity {
	Error,
	Warning
};

struct EditorDiagnostic {
	EditorRange range;
	EditorDiagnosticSeverity severity = EditorDiagnosticSeverity::Error;
	std::string message;
};

struct PartClassAutocomplete {
	int line = -1;
	int replace_start_column = 0;
	int replace_end_column = 0;
	std::string prefix;
	std::string display_prefix;
	std::vector<std::string> suggestions;
	bool active = false;
	bool stl_context = false;
	bool stl_category_context = false;
	bool shape_option_context = false;
	bool spatial_declaration_context = false;
	std::string context_label;
};

struct StlAutocompleteQuery {
	bool active = false;
	bool category_context = false;
	std::string category_prefix;
	std::string category;
	std::string name_prefix;
	std::string display_prefix;
};

struct EditorIdentifierMatchState {
	EditorRange active_range;
	std::vector<EditorRange> ranges;
	std::string text;
	bool found = false;
};

struct EditorBracketMatchState {
	EditorRange active_range;
	EditorRange matching_range;
	char delimiter = '\0';
	bool found = false;
	bool has_pair = false;
};

class GrammarEditorInteractionState
{
public:
	int cursor_column = 0;
	int selection_start = 0;
	int selection_end = 0;
	int requested_selection_start = 0;
	int requested_selection_end = 0;
	int requested_cursor_column = 0;
	int scroll_target_line = -1;
	int selected_identifier_line = -1;
	int selected_identifier_column = -1;
	bool request_focus = false;
	bool request_cursor_sync = false;
	bool request_selection_sync = false;
	bool request_scroll = false;
	bool has_selected_identifier = false;
	bool preserve_focus_through_regeneration = false;
	std::string synced_text;
	EditorToken selected_identifier_token;
};
