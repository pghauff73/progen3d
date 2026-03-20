#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cfloat>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <epoxy/gl.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Context.h"
#include "Solution.h"
#include "grammar.h"
#include "imgui_render.h"
#include "stbimage/stb_image.h"

void errorout(std::string error_str);
void debugout(const std::string &message);
void debugstate(const std::string &key, const std::string &message);
void upload_fulltext(std::string fulltext);

Grammar *grammar = nullptr;
std::string grammar_filename = "test.grammar";
std::mt19937_64 rng;

extern std::vector<Variable *> variable_list;
extern std::vector<Variable *> full_variable_list;

std::vector<int> texture_list;
std::vector<float> texture_list_alpha;
std::vector<std::string> texture_filenames;

bool loggedin = false;
bool play = false;
float scale_global = 1.0f;
float angle_view = 0.0f;
float elevation_view = 0.0f;

namespace {

struct AppState {
	std::string editor_text;
	std::string texture_path;
	float workspace_width_ratio = 0.46f;
	float preview_height_ratio = 0.68f;
};

struct AppFonts {
	ImFont *body = nullptr;
	ImFont *title = nullptr;
	ImFont *mono = nullptr;
};

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

struct EditorTokenMatch {
	int line = -1;
	int column = 0;
	bool found = false;
};

struct GrammarSymbolIndex {
	std::unordered_set<std::string> rule_names;
	std::unordered_set<std::string> variable_names;
};

struct SmartEditorState {
	int active_line = 0;
	int focus_line = 0;
	int cursor_column = 0;
	int selection_start = 0;
	int selection_end = 0;
	int requested_selection_start = 0;
	int requested_selection_end = 0;
	int requested_cursor_column = 0;
	int scroll_target_line = -1;
	int context_menu_line = -1;
	int context_menu_column = -1;
	int selected_identifier_line = -1;
	int selected_identifier_column = -1;
	bool request_focus = false;
	bool request_cursor_sync = false;
	bool request_selection_sync = false;
	bool request_scroll = false;
	bool context_menu_has_token = false;
	bool has_selected_identifier = false;
	std::string line_buffer;
	std::string synced_text;
	EditorToken context_menu_token;
	EditorToken selected_identifier_token;
};

std::mutex log_mutex;
std::vector<std::string> console_messages;
std::unordered_map<std::string, std::string> debug_state_messages;
AppFonts app_fonts;
SmartEditorState smart_editor_state;

ImVec4 color_from_hex(unsigned int rgb, float alpha = 1.0f)
{
	return ImVec4(((rgb >> 16) & 0xff) / 255.0f,
	              ((rgb >> 8) & 0xff) / 255.0f,
	              (rgb & 0xff) / 255.0f,
	              alpha);
}

void apply_professional_style()
{
	ImGuiStyle &style = ImGui::GetStyle();
	style.WindowPadding = ImVec2(12.0f, 10.0f);
	style.FramePadding = ImVec2(8.0f, 6.0f);
	style.CellPadding = ImVec2(6.0f, 5.0f);
	style.ItemSpacing = ImVec2(6.0f, 5.0f);
	style.ItemInnerSpacing = ImVec2(5.0f, 4.0f);
	style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
	style.IndentSpacing = 12.0f;
	style.ScrollbarSize = 10.0f;
	style.GrabMinSize = 8.0f;
	style.WindowBorderSize = 0.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;
	style.TabBorderSize = 0.0f;
	style.WindowRounding = 16.0f;
	style.ChildRounding = 14.0f;
	style.FrameRounding = 9.0f;
	style.PopupRounding = 11.0f;
	style.ScrollbarRounding = 12.0f;
	style.GrabRounding = 10.0f;
	style.TabRounding = 8.0f;

	ImVec4 *colors = style.Colors;
	colors[ImGuiCol_Text] = color_from_hex(0x17212b);
	colors[ImGuiCol_TextDisabled] = color_from_hex(0x7d8a98);
	colors[ImGuiCol_WindowBg] = color_from_hex(0xf4efe7);
	colors[ImGuiCol_ChildBg] = color_from_hex(0xfffbf7);
	colors[ImGuiCol_PopupBg] = color_from_hex(0xfffbf7, 0.98f);
	colors[ImGuiCol_Border] = color_from_hex(0xd9d1c6, 0.95f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	colors[ImGuiCol_FrameBg] = color_from_hex(0xede5db);
	colors[ImGuiCol_FrameBgHovered] = color_from_hex(0xe4d9cb);
	colors[ImGuiCol_FrameBgActive] = color_from_hex(0xd9ccbc);
	colors[ImGuiCol_TitleBg] = color_from_hex(0xfffbf7);
	colors[ImGuiCol_TitleBgActive] = color_from_hex(0xfffbf7);
	colors[ImGuiCol_TitleBgCollapsed] = color_from_hex(0xfffbf7);
	colors[ImGuiCol_MenuBarBg] = color_from_hex(0xfffbf7);
	colors[ImGuiCol_ScrollbarBg] = color_from_hex(0xe8dfd4);
	colors[ImGuiCol_ScrollbarGrab] = color_from_hex(0xc1b2a0);
	colors[ImGuiCol_ScrollbarGrabHovered] = color_from_hex(0xab9a85);
	colors[ImGuiCol_ScrollbarGrabActive] = color_from_hex(0x927f6b);
	colors[ImGuiCol_CheckMark] = color_from_hex(0x177d72);
	colors[ImGuiCol_SliderGrab] = color_from_hex(0x177d72);
	colors[ImGuiCol_SliderGrabActive] = color_from_hex(0x0f5d56);
	colors[ImGuiCol_Button] = color_from_hex(0x177d72);
	colors[ImGuiCol_ButtonHovered] = color_from_hex(0x1d9487);
	colors[ImGuiCol_ButtonActive] = color_from_hex(0x10655d);
	colors[ImGuiCol_Header] = color_from_hex(0xe7ddd0);
	colors[ImGuiCol_HeaderHovered] = color_from_hex(0xdcd0c0);
	colors[ImGuiCol_HeaderActive] = color_from_hex(0xd3c4b2);
	colors[ImGuiCol_Separator] = color_from_hex(0xd8d0c4);
	colors[ImGuiCol_SeparatorHovered] = color_from_hex(0x1d9487);
	colors[ImGuiCol_SeparatorActive] = color_from_hex(0x177d72);
	colors[ImGuiCol_ResizeGrip] = color_from_hex(0xb5a593, 0.45f);
	colors[ImGuiCol_ResizeGripHovered] = color_from_hex(0x1d9487, 0.75f);
	colors[ImGuiCol_ResizeGripActive] = color_from_hex(0x177d72, 0.9f);
	colors[ImGuiCol_Tab] = color_from_hex(0xe8dfd3);
	colors[ImGuiCol_TabHovered] = color_from_hex(0xd9cdbc);
	colors[ImGuiCol_TabActive] = color_from_hex(0xfffbf7);
	colors[ImGuiCol_TabUnfocused] = color_from_hex(0xe8dfd3);
	colors[ImGuiCol_TabUnfocusedActive] = color_from_hex(0xf4ede5);
	colors[ImGuiCol_PlotLines] = color_from_hex(0x177d72);
	colors[ImGuiCol_PlotLinesHovered] = color_from_hex(0x0f5d56);
	colors[ImGuiCol_PlotHistogram] = color_from_hex(0xbe7c4a);
	colors[ImGuiCol_PlotHistogramHovered] = color_from_hex(0x9f6436);
	colors[ImGuiCol_TableHeaderBg] = color_from_hex(0xefe6db);
	colors[ImGuiCol_TableBorderStrong] = color_from_hex(0xd6ccbf);
	colors[ImGuiCol_TableBorderLight] = color_from_hex(0xe4dbd0);
	colors[ImGuiCol_TableRowBg] = color_from_hex(0xfffbf7, 0.0f);
	colors[ImGuiCol_TableRowBgAlt] = color_from_hex(0xf7f1ea, 0.45f);
	colors[ImGuiCol_TextSelectedBg] = color_from_hex(0x177d72, 0.18f);
	colors[ImGuiCol_DragDropTarget] = color_from_hex(0xbe7c4a, 0.95f);
	colors[ImGuiCol_NavHighlight] = color_from_hex(0x177d72, 0.7f);
	colors[ImGuiCol_NavWindowingHighlight] = color_from_hex(0x177d72, 0.7f);
	colors[ImGuiCol_NavWindowingDimBg] = color_from_hex(0x17212b, 0.15f);
	colors[ImGuiCol_ModalWindowDimBg] = color_from_hex(0x17212b, 0.18f);
}

ImFont *try_load_font(ImGuiIO &io, const std::vector<std::string> &paths, float size_pixels)
{
	for (const std::string &path : paths) {
		if (!std::filesystem::exists(path)) {
			continue;
		}
		ImFont *font = io.Fonts->AddFontFromFileTTF(path.c_str(), size_pixels);
		if (font != nullptr) {
			return font;
		}
	}
	return nullptr;
}

void load_professional_fonts()
{
	ImGuiIO &io = ImGui::GetIO();
	io.Fonts->Clear();

	const std::vector<std::string> sans_candidates = {
		"/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
		"/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf"
	};
	const std::vector<std::string> mono_candidates = {
		"/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
		"/usr/share/fonts/truetype/liberation2/LiberationMono-Regular.ttf"
	};

	app_fonts.body = try_load_font(io, sans_candidates, 14.0f);
	app_fonts.title = try_load_font(io, sans_candidates, 20.0f);
	app_fonts.mono = try_load_font(io, mono_candidates, 13.0f);

	if (app_fonts.body == nullptr) {
		app_fonts.body = io.Fonts->AddFontDefault();
	}
	if (app_fonts.title == nullptr) {
		app_fonts.title = app_fonts.body;
	}
	if (app_fonts.mono == nullptr) {
		app_fonts.mono = app_fonts.body;
	}
}

void begin_surface(const char *id, const ImVec2 &size, bool border = true)
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, color_from_hex(0xfffbf7));
	ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xd9d1c6));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 14.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(11.0f, 11.0f));
	ImGui::BeginChild(id, size, border, ImGuiWindowFlags_NoScrollbar);
}

void end_surface()
{
	ImGui::EndChild();
	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(2);
}

void draw_stat_block(const char *label, const std::string &value, bool compact_value = false, bool wrap_value = false)
{
	ImGui::BeginGroup();
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x7d8a98));
	ImGui::TextUnformatted(label);
	ImGui::PopStyleColor();
	ImFont *value_font = compact_value ? app_fonts.body : app_fonts.title;
	if (value_font != nullptr) {
		ImGui::PushFont(value_font);
	}
	if (wrap_value) {
		ImGui::TextWrapped("%s", value.c_str());
	} else {
		ImGui::TextUnformatted(value.c_str());
	}
	if (value_font != nullptr) {
		ImGui::PopFont();
	}
	ImGui::EndGroup();
}

void draw_divider()
{
	ImGui::PushStyleColor(ImGuiCol_Separator, color_from_hex(0xdfd6ca));
	ImGui::Separator();
	ImGui::PopStyleColor();
}

void draw_splitter(const char *id,
                   bool vertical,
                   float thickness,
                   float length,
                   float *primary_size,
                   float *secondary_size,
                   float min_primary,
                   float min_secondary)
{
	if (primary_size == nullptr || secondary_size == nullptr) {
		return;
	}

	ImVec2 button_size = vertical ? ImVec2(thickness, length) : ImVec2(length, thickness);
	ImGui::PushStyleColor(ImGuiCol_Button, color_from_hex(0xf0e8de));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color_from_hex(0xe1d5c4));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, color_from_hex(0xd4c3ad));
	ImGui::Button(id, button_size);
	ImGui::PopStyleColor(3);

	if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
		ImGui::SetMouseCursor(vertical ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS);
	}

	if (!ImGui::IsItemActive()) {
		return;
	}

	float delta = vertical ? ImGui::GetIO().MouseDelta.x : ImGui::GetIO().MouseDelta.y;
	if (delta == 0.0f) {
		return;
	}

	float new_primary = *primary_size + delta;
	float new_secondary = *secondary_size - delta;

	if (new_primary < min_primary) {
		const float correction = min_primary - new_primary;
		new_primary += correction;
		new_secondary -= correction;
	}
	if (new_secondary < min_secondary) {
		const float correction = min_secondary - new_secondary;
		new_secondary += correction;
		new_primary -= correction;
	}

	*primary_size = std::max(min_primary, new_primary);
	*secondary_size = std::max(min_secondary, new_secondary);
}

void append_console_message(std::string message)
{
	if (message.empty()) {
		return;
	}
	if (message.back() == '\n') {
		message.pop_back();
	}

	std::lock_guard<std::mutex> lock(log_mutex);
	console_messages.push_back(std::move(message));
	if (console_messages.size() > 2000) {
		console_messages.erase(console_messages.begin(),
		                       console_messages.begin() + 500);
	}
}

void clear_variable_pool()
{
	std::unordered_set<Variable *> unique_variables;
	for (Variable *variable : variable_list) {
		if (variable != nullptr) {
			unique_variables.insert(variable);
		}
	}
	for (Variable *variable : full_variable_list) {
		if (variable != nullptr) {
			unique_variables.insert(variable);
		}
	}
	for (Variable *variable : unique_variables) {
		delete variable;
	}
	variable_list.clear();
	full_variable_list.clear();
}

void destroy_grammar()
{
	if (grammar != nullptr) {
		delete grammar;
		grammar = nullptr;
	}
}

void destroy_textures()
{
	for (int texture : texture_list) {
		if (texture != 0) {
			GLuint handle = static_cast<GLuint>(texture);
			glDeleteTextures(1, &handle);
		}
	}
	texture_list.clear();
	texture_list_alpha.clear();
	texture_filenames.clear();
}

bool write_text_file(const std::string &path, const std::string &text)
{
	std::ofstream fout(path);
	if (!fout.is_open()) {
		return false;
	}
	fout << text;
	return true;
}

bool read_text_file(const std::string &path, std::string *text)
{
	if (text == nullptr) {
		return false;
	}

	std::ifstream fin(path);
	if (!fin.is_open()) {
		return false;
	}

	std::stringstream buffer;
	buffer << fin.rdbuf();
	*text = buffer.str();
	return true;
}

std::vector<std::string> breakup_into_lines(std::string input, std::string delimiter)
{
	std::vector<std::string> output;
	int pos = -1;
	while ((pos = input.find(delimiter)) != -1) {
		output.push_back(input.substr(0, pos));
		input.erase(0, pos + delimiter.length());
	}
	output.push_back(input);
	return output;
}

std::string join_lines(const std::vector<std::string> &lines)
{
	std::ostringstream output;
	for (std::size_t i = 0; i < lines.size(); ++i) {
		if (i != 0) {
			output << '\n';
		}
		output << lines[i];
	}
	return output.str();
}

bool is_identifier_start_char(char c)
{
	return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
}

bool is_identifier_char(char c)
{
	return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
}

bool is_number_start(const std::string &text, std::size_t index)
{
	if (index >= text.size()) {
		return false;
	}
	const char current = text[index];
	if (std::isdigit(static_cast<unsigned char>(current)) != 0) {
		return true;
	}
	if (current == '.' && index + 1 < text.size() &&
	    std::isdigit(static_cast<unsigned char>(text[index + 1])) != 0) {
		return true;
	}
	if ((current == '-' || current == '+') && index + 1 < text.size()) {
		const char next = text[index + 1];
		if (std::isdigit(static_cast<unsigned char>(next)) != 0 || next == '.') {
			if (index == 0) {
				return true;
			}
			const char prev = text[index - 1];
			return std::isspace(static_cast<unsigned char>(prev)) != 0 ||
			       prev == '(' || prev == '[' || prev == '{' || prev == ',' || prev == ';';
		}
	}
	return false;
}

std::size_t consume_number_token(const std::string &text, std::size_t index)
{
	if (index < text.size() && (text[index] == '-' || text[index] == '+')) {
		++index;
	}
	bool seen_dot = false;
	while (index < text.size()) {
		const char c = text[index];
		if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
			++index;
			continue;
		}
		if (c == '.' && !seen_dot) {
			seen_dot = true;
			++index;
			continue;
		}
		break;
	}
	return index;
}

std::size_t find_comment_start(const std::string &line)
{
	const std::size_t hash_comment = line.find('#');
	const std::size_t slash_comment = line.find("//");
	if (hash_comment == std::string::npos) {
		return slash_comment;
	}
	if (slash_comment == std::string::npos) {
		return hash_comment;
	}
	return std::min(hash_comment, slash_comment);
}

GrammarSymbolIndex build_grammar_symbol_index(const std::vector<std::string> &lines)
{
	GrammarSymbolIndex symbols;
	const std::regex rule_re("^\\s*([A-Za-z_][A-Za-z0-9_]*)\\b.*->");
	const std::regex variable_decl_re("\\bR\\*?\\s+([A-Za-z_][A-Za-z0-9_]*)\\b");

	for (const std::string &line : lines) {
		std::smatch match;
		if (std::regex_search(line, match, rule_re)) {
			symbols.rule_names.insert(match[1].str());
		}
		auto search_start = line.cbegin();
		while (std::regex_search(search_start, line.cend(), match, variable_decl_re)) {
			symbols.variable_names.insert(match[1].str());
			search_start = match.suffix().first;
		}
	}

	return symbols;
}

ImU32 color_for_editor_token(EditorTokenKind kind)
{
	switch (kind) {
	case EditorTokenKind::Comment:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0x7d8a98));
	case EditorTokenKind::RuleDefinition:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0x0f5d56));
	case EditorTokenKind::RuleReference:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0x177d72));
	case EditorTokenKind::Variable:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0xa25b1f));
	case EditorTokenKind::Number:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0xbe7c4a));
	case EditorTokenKind::Operator:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0x7d4aa3));
	case EditorTokenKind::Keyword:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0x355c9b));
	case EditorTokenKind::Primitive:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0x8f5a2d));
	case EditorTokenKind::Identifier:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0x44515f));
	case EditorTokenKind::Plain:
	default:
		return ImGui::GetColorU32(ImGuiCol_Text);
	}
}

std::string tooltip_for_operator(const std::string &token)
{
	if (token == "->") {
		return "Production operator. The symbols on the left expand into the sequence on the right.";
	}
	if (token == "|") {
		return "Alternative branch. The grammar may choose between adjacent productions.";
	}
	if (token == "[") {
		return "Branch start. Opens a scoped transform or generation block.";
	}
	if (token == "]") {
		return "Branch end. Closes the current scoped generation block.";
	}
	if (token == "(" || token == ")") {
		return "Argument group delimiter.";
	}
	if (token == ";") {
		return "Weighted-choice separator.";
	}
	if (token == "+" || token == "-" || token == "*" || token == "/") {
		return "Arithmetic operator used inside grammar expressions.";
	}
	return "Grammar operator.";
}

std::string tooltip_for_keyword(const std::string &token)
{
	if (token == "R") {
		return "Random variable declaration. Assigns a sampled value into a named symbol.";
	}
	if (token == "R*") {
		return "Random integer-like variable declaration. Often used for texture or rule indices.";
	}
	if (token == "S") {
		return "Scale transform.";
	}
	if (token == "T") {
		return "Translate transform.";
	}
	if (token == "I") {
		return "Instantiate primitive geometry.";
	}
	if (token == "A") {
		return "Grammar opcode.";
	}
	if (token == "D") {
		return "Grammar opcode.";
	}
	return "Grammar keyword.";
}

bool is_selectable_identifier_token(EditorTokenKind kind)
{
	return kind == EditorTokenKind::Identifier ||
	       kind == EditorTokenKind::Variable ||
	       kind == EditorTokenKind::RuleDefinition ||
	       kind == EditorTokenKind::RuleReference ||
	       kind == EditorTokenKind::Primitive;
}

std::string label_for_editor_token_kind(EditorTokenKind kind)
{
	switch (kind) {
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

std::vector<EditorToken> tokenize_grammar_line(const std::string &line, const GrammarSymbolIndex &symbols)
{
	std::vector<EditorToken> tokens;
	const std::unordered_set<std::string> keywords = {"R", "R*", "S", "T", "I", "A", "D"};
	const std::unordered_set<std::string> primitives = {"Cube", "CubeX", "CubeY"};

	std::string definition_name;
	const std::regex rule_re("^\\s*([A-Za-z_][A-Za-z0-9_]*)\\b.*->");
	std::smatch match;
	if (std::regex_search(line, match, rule_re)) {
		definition_name = match[1].str();
	}

	const std::size_t comment_start = find_comment_start(line);
	const std::string code = line.substr(0, comment_start);
	std::size_t index = 0;
	while (index < code.size()) {
		const char c = code[index];
		if (std::isspace(static_cast<unsigned char>(c)) != 0) {
			const std::size_t start = index;
			while (index < code.size() &&
			       std::isspace(static_cast<unsigned char>(code[index])) != 0) {
				++index;
			}
			tokens.push_back({code.substr(start, index - start), EditorTokenKind::Plain, ""});
			continue;
		}

		if (code.compare(index, 2, "->") == 0) {
			tokens.push_back({"->", EditorTokenKind::Operator, tooltip_for_operator("->")});
			index += 2;
			continue;
		}

		if (code.compare(index, 2, "R*") == 0 &&
		    (index + 2 >= code.size() || !is_identifier_char(code[index + 2]))) {
			tokens.push_back({"R*", EditorTokenKind::Keyword, tooltip_for_keyword("R*")});
			index += 2;
			continue;
		}

		if (is_number_start(code, index)) {
			const std::size_t end = consume_number_token(code, index);
			tokens.push_back({code.substr(index, end - index),
			                 EditorTokenKind::Number,
			                 "Numeric literal used by a grammar expression or transform."});
			index = end;
			continue;
		}

		if (is_identifier_start_char(c)) {
			const std::size_t start = index;
			while (index < code.size() && is_identifier_char(code[index])) {
				++index;
			}
			const std::string identifier = code.substr(start, index - start);
			EditorTokenKind kind = EditorTokenKind::Identifier;
			std::string tooltip = "Identifier.";
			if (identifier == definition_name) {
				kind = EditorTokenKind::RuleDefinition;
				tooltip = "Rule definition. This line declares how this grammar symbol expands.";
			} else if (keywords.count(identifier) != 0U) {
				kind = EditorTokenKind::Keyword;
				tooltip = tooltip_for_keyword(identifier);
			} else if (primitives.count(identifier) != 0U) {
				kind = EditorTokenKind::Primitive;
				tooltip = "Primitive mesh symbol used by the geometry instancer.";
			} else if (symbols.variable_names.count(identifier) != 0U) {
				kind = EditorTokenKind::Variable;
				tooltip = "Grammar variable. Hovered symbol matches a declared runtime value.";
			} else if (symbols.rule_names.count(identifier) != 0U) {
				kind = EditorTokenKind::RuleReference;
				tooltip = "Rule reference. This symbol expands using another grammar production.";
			}
			tokens.push_back({identifier, kind, tooltip});
			continue;
		}

		if (std::string("|[]{}();,+-*/").find(c) != std::string::npos) {
			const std::string token(1, c);
			tokens.push_back({token, EditorTokenKind::Operator, tooltip_for_operator(token)});
			++index;
			continue;
		}

		tokens.push_back({std::string(1, c), EditorTokenKind::Plain, ""});
		++index;
	}

	if (comment_start != std::string::npos) {
		tokens.push_back({line.substr(comment_start),
		                 EditorTokenKind::Comment,
		                 "Comment. Ignored by the grammar parser."});
	}

	if (tokens.empty()) {
		tokens.push_back({"", EditorTokenKind::Plain, ""});
	}
	return tokens;
}

struct EditorLineInputCallbackUserData {
	std::string *text = nullptr;
	int *cursor_column = nullptr;
	int *selection_start = nullptr;
	int *selection_end = nullptr;
};

int editor_line_input_callback(ImGuiInputTextCallbackData *data)
{
	EditorLineInputCallbackUserData *user_data =
		static_cast<EditorLineInputCallbackUserData *>(data->UserData);
	if (user_data == nullptr || user_data->text == nullptr) {
		return 0;
	}
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize) {
		user_data->text->resize(static_cast<std::size_t>(data->BufTextLen));
		data->Buf = user_data->text->data();
		return 0;
	}
	if (data->EventFlag == ImGuiInputTextFlags_CallbackAlways) {
		if (smart_editor_state.request_cursor_sync) {
			const int target_column =
				std::clamp(smart_editor_state.requested_cursor_column, 0, data->BufTextLen);
			data->CursorPos = target_column;
			smart_editor_state.request_cursor_sync = false;
		}
		if (smart_editor_state.request_selection_sync) {
			data->SelectionStart =
				std::clamp(smart_editor_state.requested_selection_start, 0, data->BufTextLen);
			data->SelectionEnd =
				std::clamp(smart_editor_state.requested_selection_end, 0, data->BufTextLen);
			smart_editor_state.request_selection_sync = false;
		}
		if (user_data->cursor_column != nullptr) {
			*user_data->cursor_column = data->CursorPos;
		}
		if (user_data->selection_start != nullptr) {
			*user_data->selection_start = data->SelectionStart;
		}
		if (user_data->selection_end != nullptr) {
			*user_data->selection_end = data->SelectionEnd;
		}
	}
	return 0;
}

std::vector<std::string> current_editor_lines(const std::string &text)
{
	std::vector<std::string> lines = breakup_into_lines(text, "\n");
	if (lines.empty()) {
		lines.push_back("");
	}
	return lines;
}

void set_active_editor_line(const std::vector<std::string> &lines, int line_index, bool request_focus)
{
	if (lines.empty()) {
		smart_editor_state.active_line = 0;
		smart_editor_state.line_buffer.clear();
		smart_editor_state.cursor_column = 0;
		smart_editor_state.selection_start = 0;
		smart_editor_state.selection_end = 0;
		smart_editor_state.request_focus = request_focus;
		smart_editor_state.focus_line = 0;
		return;
	}
	line_index = std::clamp(line_index, 0, static_cast<int>(lines.size()) - 1);
	smart_editor_state.active_line = line_index;
	smart_editor_state.focus_line = line_index;
	smart_editor_state.request_focus = request_focus;
	smart_editor_state.line_buffer = lines[static_cast<std::size_t>(line_index)];
	smart_editor_state.cursor_column =
		std::min(smart_editor_state.cursor_column, static_cast<int>(smart_editor_state.line_buffer.size()));
	smart_editor_state.selection_start = smart_editor_state.cursor_column;
	smart_editor_state.selection_end = smart_editor_state.cursor_column;
}

void jump_to_editor_location(const std::vector<std::string> &lines, int line_index, int column)
{
	if (lines.empty()) {
		set_active_editor_line(lines, 0, true);
		smart_editor_state.requested_cursor_column = 0;
		smart_editor_state.request_cursor_sync = true;
		smart_editor_state.request_scroll = true;
		smart_editor_state.scroll_target_line = 0;
		return;
	}

	line_index = std::clamp(line_index, 0, static_cast<int>(lines.size()) - 1);
	const int clamped_column =
		std::clamp(column, 0, static_cast<int>(lines[static_cast<std::size_t>(line_index)].size()));
	smart_editor_state.cursor_column = clamped_column;
	set_active_editor_line(lines, line_index, true);
	smart_editor_state.cursor_column = clamped_column;
	smart_editor_state.selection_start = clamped_column;
	smart_editor_state.selection_end = clamped_column;
	smart_editor_state.requested_cursor_column = clamped_column;
	smart_editor_state.request_cursor_sync = true;
	smart_editor_state.requested_selection_start = clamped_column;
	smart_editor_state.requested_selection_end = clamped_column;
	smart_editor_state.request_selection_sync = true;
	smart_editor_state.request_scroll = true;
	smart_editor_state.scroll_target_line = line_index;
}

void select_editor_range(const std::vector<std::string> &lines, int line_index, int start_column, int end_column)
{
	if (lines.empty()) {
		jump_to_editor_location(lines, 0, 0);
		return;
	}

	line_index = std::clamp(line_index, 0, static_cast<int>(lines.size()) - 1);
	const int line_length = static_cast<int>(lines[static_cast<std::size_t>(line_index)].size());
	const int clamped_start = std::clamp(start_column, 0, line_length);
	const int clamped_end = std::clamp(end_column, clamped_start, line_length);

	smart_editor_state.cursor_column = clamped_end;
	set_active_editor_line(lines, line_index, true);
	smart_editor_state.cursor_column = clamped_end;
	smart_editor_state.selection_start = clamped_start;
	smart_editor_state.selection_end = clamped_end;
	smart_editor_state.requested_cursor_column = clamped_end;
	smart_editor_state.requested_selection_start = clamped_start;
	smart_editor_state.requested_selection_end = clamped_end;
	smart_editor_state.request_cursor_sync = true;
	smart_editor_state.request_selection_sync = true;
	smart_editor_state.request_scroll = true;
	smart_editor_state.scroll_target_line = line_index;
}

EditorTokenMatch find_next_token_match(const std::vector<std::string> &lines,
                                       const GrammarSymbolIndex &symbols,
                                       const EditorToken &needle,
                                       int origin_line,
                                       int origin_column)
{
	if (needle.text.empty() || lines.empty()) {
		return {};
	}

	const int line_count = static_cast<int>(lines.size());
	for (int pass = 0; pass < 2; ++pass) {
		const int start_line = pass == 0 ? std::clamp(origin_line, 0, line_count - 1) : 0;
		const int end_line = pass == 0 ? line_count : std::clamp(origin_line, 0, line_count - 1) + 1;
		for (int line_index = start_line; line_index < end_line; ++line_index) {
			const std::vector<EditorToken> tokens =
				tokenize_grammar_line(lines[static_cast<std::size_t>(line_index)], symbols);
			int column = 0;
			for (const EditorToken &token : tokens) {
				const int token_column = column;
				column += static_cast<int>(token.text.size());
				if (token.text != needle.text) {
					continue;
				}
				if (line_index == origin_line && token_column <= origin_column) {
					continue;
				}
				return {line_index, token_column, true};
			}
		}
	}

	return {};
}

EditorTokenMatch find_next_rule_arrow(const std::vector<std::string> &lines, int origin_line, int origin_column)
{
	if (lines.empty()) {
		return {};
	}

	const int line_count = static_cast<int>(lines.size());
	origin_line = std::clamp(origin_line, 0, line_count - 1);
	for (int pass = 0; pass < 2; ++pass) {
		const int start_line = pass == 0 ? origin_line : 0;
		const int end_line = pass == 0 ? line_count : origin_line + 1;
		for (int line_index = start_line; line_index < end_line; ++line_index) {
			std::size_t search_from = 0;
			if (line_index == origin_line) {
				search_from = static_cast<std::size_t>(std::max(origin_column + 1, 0));
			}
			const std::size_t match = lines[static_cast<std::size_t>(line_index)].find("->", search_from);
			if (match != std::string::npos) {
				return {line_index, static_cast<int>(match), true};
			}
		}
	}

	return {};
}

EditorTokenMatch find_previous_rule_arrow(const std::vector<std::string> &lines, int origin_line, int origin_column)
{
	if (lines.empty()) {
		return {};
	}

	const int line_count = static_cast<int>(lines.size());
	origin_line = std::clamp(origin_line, 0, line_count - 1);
	for (int pass = 0; pass < 2; ++pass) {
		const int start_line = pass == 0 ? origin_line : line_count - 1;
		const int end_line = pass == 0 ? -1 : origin_line - 1;
		for (int line_index = start_line; line_index > end_line; --line_index) {
			const std::string &line = lines[static_cast<std::size_t>(line_index)];
			std::size_t search_limit = line.size();
			if (line_index == origin_line) {
				search_limit = static_cast<std::size_t>(std::clamp(origin_column, 0, static_cast<int>(line.size())));
			}
			if (search_limit == 0) {
				continue;
			}
			const std::size_t match = line.rfind("->", search_limit - 1);
			if (match != std::string::npos && match < search_limit) {
				return {line_index, static_cast<int>(match), true};
			}
		}
	}

	return {};
}

int count_token_occurrences(const std::vector<std::string> &lines,
                            const GrammarSymbolIndex &symbols,
                            const EditorToken &needle)
{
	if (needle.text.empty()) {
		return 0;
	}

	int count = 0;
	for (const std::string &line : lines) {
		const std::vector<EditorToken> tokens = tokenize_grammar_line(line, symbols);
		for (const EditorToken &token : tokens) {
			if (token.text == needle.text && token.kind == needle.kind) {
				++count;
			}
		}
	}
	return count;
}

void sync_smart_editor_state(const std::string &text)
{
	if (smart_editor_state.synced_text == text) {
		return;
	}
	std::vector<std::string> lines = current_editor_lines(text);
	if (smart_editor_state.active_line >= static_cast<int>(lines.size())) {
		smart_editor_state.active_line = static_cast<int>(lines.size()) - 1;
	}
	if (smart_editor_state.active_line < 0) {
		smart_editor_state.active_line = 0;
	}
	smart_editor_state.line_buffer = lines[static_cast<std::size_t>(smart_editor_state.active_line)];
	smart_editor_state.cursor_column =
		std::min(smart_editor_state.cursor_column, static_cast<int>(smart_editor_state.line_buffer.size()));
	smart_editor_state.selection_start = smart_editor_state.cursor_column;
	smart_editor_state.selection_end = smart_editor_state.cursor_column;
	smart_editor_state.synced_text = text;
}

void commit_editor_lines(AppState *app_state, const std::vector<std::string> &lines)
{
	if (app_state == nullptr) {
		return;
	}
	app_state->editor_text = join_lines(lines);
	smart_editor_state.synced_text = app_state->editor_text;
}

int input_text_resize_callback(ImGuiInputTextCallbackData *data)
{
	if (data->EventFlag != ImGuiInputTextFlags_CallbackResize) {
		return 0;
	}
	std::string *text = static_cast<std::string *>(data->UserData);
	text->resize(static_cast<std::size_t>(data->BufTextLen));
	data->Buf = text->data();
	return 0;
}

bool input_text_string(const char *label,
                       std::string *text,
                       ImGuiInputTextFlags flags = 0)
{
	if (text == nullptr) {
		return false;
	}
	if (text->capacity() < 256) {
		text->reserve(256);
	}
	flags |= ImGuiInputTextFlags_CallbackResize;
	return ImGui::InputText(label,
	                        text->data(),
	                        text->capacity() + 1,
	                        flags,
	                        input_text_resize_callback,
	                        text);
}

bool input_text_multiline_string(const char *label,
                                 std::string *text,
                                 const ImVec2 &size,
                                 ImGuiInputTextFlags flags = 0)
{
	if (text == nullptr) {
		return false;
	}
	if (text->capacity() < 1024) {
		text->reserve(1024);
	}
	flags |= ImGuiInputTextFlags_CallbackResize;
	return ImGui::InputTextMultiline(label,
	                                 text->data(),
	                                 text->capacity() + 1,
	                                 size,
	                                 flags,
	                                 input_text_resize_callback,
	                                 text);
}

GLuint generate_texture(const std::string &filename)
{
	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char *image = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	if (image == nullptr) {
		errorout(std::string("Failed to load texture: ") + filename + " : " + stbi_failure_reason());
		return 0;
	}

	GLuint texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glBindTexture(GL_TEXTURE_2D, 0);

	stbi_image_free(image);
	return texture;
}

bool add_texture_file(const std::string &filename)
{
	if (filename.empty()) {
		errorout("Texture path is empty");
		return false;
	}
	if (!std::filesystem::exists(filename)) {
		errorout("Texture file does not exist: " + filename);
		return false;
	}

	const GLuint texture = generate_texture(filename);
	if (texture == 0) {
		return false;
	}

	texture_list.push_back(static_cast<int>(texture));
	texture_list_alpha.push_back(1.0f);
	texture_filenames.push_back(filename);
	debugout("Loaded texture: " + filename);
	return true;
}

void load_default_textures()
{
	if (!std::filesystem::exists("textures")) {
		debugout("No textures directory found");
		return;
	}

	std::vector<std::string> files;
	for (const auto &entry : std::filesystem::directory_iterator("textures")) {
		if (!entry.is_regular_file()) {
			continue;
		}
		std::string extension = entry.path().extension().string();
		std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
		});
		if (extension == ".png" || extension == ".jpg" || extension == ".jpeg") {
			files.push_back(entry.path().string());
		}
	}

	std::sort(files.begin(), files.end());
	for (const std::string &file : files) {
		add_texture_file(file);
	}
}

bool regenerate_scene(const std::string &source_text)
{
	try {
		const std::filesystem::path temp_path =
			std::filesystem::temp_directory_path() / "progen3d-imgui-current.grammar";
		if (!write_text_file(temp_path.string(), source_text)) {
			errorout("Failed to write temporary grammar file");
			return false;
		}

		clear_variable_pool();
		std::unique_ptr<Grammar> next_grammar(new Grammar(temp_path.string()));
		next_grammar->lines = breakup_into_lines(source_text, "\n");
		if (!next_grammar->lines.empty()) {
			next_grammar->Reread();
		}
		next_grammar->addContext();
		for (int texture : texture_list) {
			next_grammar->context->loadTexture(static_cast<GLuint>(texture));
		}
		next_grammar->context->genPrimitives();
		next_grammar->generateGeometry();

		auto base_vertices = build_base_vertex_data();
		std::vector<std::vector<GLfloat>> buffers;
		std::vector<int> counts;
		next_grammar->context->buildTextureBuffers(base_vertices.data(),
		                                          texture_list.size(),
		                                          &buffers,
		                                          &counts);
		upload_render_buffers(buffers, counts, texture_list.size());

		destroy_grammar();
		grammar = next_grammar.release();
		debugout("Regenerated scene");
		return true;
	}
	catch (const std::exception &exception) {
		errorout(std::string("Regeneration failed: ") + exception.what());
		return false;
	}
	catch (...) {
		errorout("Regeneration failed");
		return false;
	}
}

bool load_grammar_into_editor(AppState *app_state, const std::string &path)
{
	if (app_state == nullptr) {
		return false;
	}

	std::string text;
	if (!read_text_file(path, &text)) {
		errorout("Could not open grammar file: " + path);
		return false;
	}

	app_state->editor_text = text;
	grammar_filename = path;
	debugout("Loaded grammar: " + path);
	return true;
}

void export_ply(const std::string &path)
{
	if (grammar == nullptr || grammar->context == nullptr) {
		errorout("No generated grammar context to export");
		return;
	}

	auto base_vertices = build_base_vertex_data();
	grammar->context->PLY(base_vertices.data(), path);
	debugout("Exported PLY: " + path);
}

void draw_texture_controls(AppState *app_state)
{
	ImGui::TextUnformatted("Texture Library");
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x7d8a98));
	ImGui::TextWrapped("Manage texture order and opacity used during procedural regeneration.");
	ImGui::PopStyleColor();
	draw_divider();

	input_text_string("##texture_path", &app_state->texture_path);
	ImGui::SameLine();
	if (ImGui::Button("Add Texture")) {
		if (add_texture_file(app_state->texture_path)) {
			app_state->texture_path.clear();
		}
	}

	bool needs_regeneration = false;
	for (std::size_t i = 0; i < texture_filenames.size(); ++i) {
		ImGui::PushID(static_cast<int>(i));
		ImGui::PushStyleColor(ImGuiCol_ChildBg, color_from_hex(0xf6f0e8));
		ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xe0d6ca));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
		ImGui::BeginChild("TextureCard", ImVec2(0.0f, 82.0f), true);
		if (i < texture_list.size()) {
			ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(texture_list[i])),
			             ImVec2(56.0f, 56.0f),
			             ImVec2(0.0f, 1.0f),
			             ImVec2(1.0f, 0.0f));
		}
		ImGui::SameLine();
		ImGui::BeginGroup();
		const std::string texture_name = std::filesystem::path(texture_filenames[i]).filename().string();
		ImGui::TextUnformatted(texture_name.c_str());
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x7d8a98));
		ImGui::TextWrapped("%s", texture_filenames[i].c_str());
		ImGui::PopStyleColor();
		if (i < texture_list_alpha.size()) {
			ImGui::SliderFloat("Alpha", &texture_list_alpha[i], 0.0f, 1.0f, "%.2f");
		}
		if (ImGui::Button("Up") && i > 0) {
			std::swap(texture_list[i], texture_list[i - 1]);
			std::swap(texture_list_alpha[i], texture_list_alpha[i - 1]);
			std::swap(texture_filenames[i], texture_filenames[i - 1]);
			needs_regeneration = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Down") && i + 1 < texture_filenames.size()) {
			std::swap(texture_list[i], texture_list[i + 1]);
			std::swap(texture_list_alpha[i], texture_list_alpha[i + 1]);
			std::swap(texture_filenames[i], texture_filenames[i + 1]);
			needs_regeneration = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove")) {
			GLuint texture = static_cast<GLuint>(texture_list[i]);
			glDeleteTextures(1, &texture);
			texture_list.erase(texture_list.begin() + static_cast<long>(i));
			texture_list_alpha.erase(texture_list_alpha.begin() + static_cast<long>(i));
			texture_filenames.erase(texture_filenames.begin() + static_cast<long>(i));
			needs_regeneration = true;
			ImGui::EndGroup();
			ImGui::EndChild();
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(2);
			ImGui::PopID();
			break;
		}
		ImGui::EndGroup();
		ImGui::EndChild();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(2);
		ImGui::PopID();
	}

	if (needs_regeneration) {
		regenerate_scene(app_state->editor_text);
	}
}

void draw_console()
{
	ImGui::TextUnformatted("Runtime Console");
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x7d8a98));
	ImGui::TextUnformatted("Logs, parser output, and runtime diagnostics.");
	ImGui::PopStyleColor();
	draw_divider();
	ImGui::BeginChild("ConsoleScroll");
	ImGuiListClipper clipper;
	std::lock_guard<std::mutex> lock(log_mutex);
	clipper.Begin(static_cast<int>(console_messages.size()));
	if (app_fonts.mono != nullptr) {
		ImGui::PushFont(app_fonts.mono);
	}
	while (clipper.Step()) {
		for (int line = clipper.DisplayStart; line < clipper.DisplayEnd; ++line) {
			const std::string &message = console_messages[static_cast<std::size_t>(line)];
			if (message.rfind("[DEBUG]", 0) == 0) {
				ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x5c6c7b));
				ImGui::TextWrapped("%s", message.c_str());
				ImGui::PopStyleColor();
			} else {
				ImGui::TextWrapped("%s", message.c_str());
			}
		}
	}
	if (app_fonts.mono != nullptr) {
		ImGui::PopFont();
	}
	if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
		ImGui::SetScrollHereY(1.0f);
	}
	ImGui::EndChild();
}

void draw_preview(AppState *app_state, float delta_seconds)
{
	if (play) {
		angle_view += delta_seconds * 35.0f;
		if (angle_view >= 360.0f) {
			angle_view -= 360.0f;
		}
	}

	ImGui::TextUnformatted("Preview Stage");
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x7d8a98));
	ImGui::TextUnformatted("Inspect the generated structure and tune the camera rail.");
	ImGui::PopStyleColor();
	draw_divider();
	if (ImGui::Button(play ? "Pause" : "Play")) {
		play = !play;
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset View")) {
		play = false;
		scale_global = 1.0f;
		angle_view = 0.0f;
		elevation_view = 0.0f;
	}
	ImGui::SameLine();
	if (ImGui::Button("Run")) {
		regenerate_scene(app_state->editor_text);
	}

	draw_divider();

	ImVec2 available = ImGui::GetContentRegionAvail();
	available.x = std::max(available.x, 100.0f);
	available.y = std::max(available.y, 100.0f);

	render_scene_to_preview(static_cast<int>(available.x),
	                        static_cast<int>(available.y),
	                        texture_list,
	                        texture_list_alpha,
	                        scale_global,
	                        angle_view,
	                        elevation_view);

	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xd6ccbf));
	ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(preview_texture_id())),
	             available,
	             ImVec2(0.0f, 1.0f),
	             ImVec2(1.0f, 0.0f));
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();

	if (ImGui::IsItemHovered()) {
		ImGuiIO &io = ImGui::GetIO();
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			angle_view += io.MouseDelta.x * 0.35f;
			elevation_view += io.MouseDelta.y * 0.35f;
			elevation_view = std::clamp(elevation_view, -89.0f, 89.0f);
		}
		if (io.MouseWheel != 0.0f) {
			scale_global = std::clamp(scale_global + io.MouseWheel * 0.05f, 0.2f, 3.0f);
		}
	}
}

void draw_smart_editor(AppState *app_state)
{
	if (app_state == nullptr) {
		return;
	}

	sync_smart_editor_state(app_state->editor_text);
	std::vector<std::string> lines = current_editor_lines(app_state->editor_text);
	if (smart_editor_state.active_line >= 0 &&
	    smart_editor_state.active_line < static_cast<int>(lines.size())) {
		lines[static_cast<std::size_t>(smart_editor_state.active_line)] = smart_editor_state.line_buffer;
	}
	const GrammarSymbolIndex symbols = build_grammar_symbol_index(lines);

	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x7d8a98));
	ImGui::TextUnformatted("Hover tokens for grammar hints. Click a line to edit it.");
	ImGui::PopStyleColor();
	ImGui::Spacing();

	const ImGuiWindowFlags editor_flags = ImGuiWindowFlags_HorizontalScrollbar;
	ImGui::BeginChild("SmartEditorRegion", ImVec2(0.0f, 0.0f), true, editor_flags);
	if (app_fonts.mono != nullptr) {
		ImGui::PushFont(app_fonts.mono);
	}

	const float line_height = ImGui::GetTextLineHeightWithSpacing() + 4.0f;
	const float gutter_width = 48.0f;
	if (smart_editor_state.request_scroll && smart_editor_state.scroll_target_line >= 0) {
		const float visible_height = std::max(ImGui::GetWindowHeight() - (ImGui::GetStyle().WindowPadding.y * 2.0f), line_height);
		const float target_center =
			(static_cast<float>(smart_editor_state.scroll_target_line) * line_height) - (visible_height * 0.35f);
		ImGui::SetScrollY(std::max(0.0f, target_center));
		smart_editor_state.request_scroll = false;
		smart_editor_state.scroll_target_line = -1;
	}

	if (ImGui::BeginTable("SmartEditorTable",
	                      2,
	                      ImGuiTableFlags_SizingFixedFit |
	                          ImGuiTableFlags_RowBg |
	                          ImGuiTableFlags_NoPadInnerX |
	                          ImGuiTableFlags_BordersInnerV)) {
		ImGui::TableSetupColumn("LineNumbers", ImGuiTableColumnFlags_WidthFixed, gutter_width);
		ImGui::TableSetupColumn("Code", ImGuiTableColumnFlags_WidthStretch);

		ImGuiListClipper clipper;
		clipper.Begin(static_cast<int>(lines.size()), line_height);
		while (clipper.Step()) {
			for (int line_index = clipper.DisplayStart; line_index < clipper.DisplayEnd; ++line_index) {
				ImGui::TableNextRow(ImGuiTableRowFlags_None, line_height);

				ImGui::TableSetColumnIndex(0);
				const bool is_active_line = line_index == smart_editor_state.active_line;
				ImGui::PushStyleColor(ImGuiCol_Text, is_active_line ? color_from_hex(0x177d72) : color_from_hex(0x7d8a98));
				char line_number[16];
				std::snprintf(line_number, sizeof(line_number), "%d", line_index + 1);
				const float gutter_x = ImGui::GetCursorPosX();
				const float line_number_width = ImGui::CalcTextSize(line_number).x;
				ImGui::SetCursorPosX(gutter_x + std::max(0.0f, gutter_width - line_number_width - 8.0f));
				ImGui::TextUnformatted(line_number);
				ImGui::PopStyleColor();

				ImGui::TableSetColumnIndex(1);
				ImDrawList *draw_list = ImGui::GetWindowDrawList();
				const ImVec2 row_min = ImGui::GetCursorScreenPos();
				const ImVec2 row_size(std::max(ImGui::GetContentRegionAvail().x, 64.0f), line_height);
				const std::string row_id = "##editor_row_" + std::to_string(line_index);

				if (is_active_line) {
					if (smart_editor_state.line_buffer.capacity() < lines[static_cast<std::size_t>(line_index)].size() + 64) {
						smart_editor_state.line_buffer.reserve(lines[static_cast<std::size_t>(line_index)].size() + 64);
					}
					ImGui::PushStyleColor(ImGuiCol_FrameBg, color_from_hex(0xf2ece3));
					ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0x177d72, 0.45f));
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (smart_editor_state.request_focus && smart_editor_state.focus_line == line_index) {
						ImGui::SetKeyboardFocusHere();
						smart_editor_state.request_focus = false;
					}
					EditorLineInputCallbackUserData callback_data{
						&smart_editor_state.line_buffer,
						&smart_editor_state.cursor_column,
						&smart_editor_state.selection_start,
						&smart_editor_state.selection_end};
					const ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_CallbackResize |
					                                        ImGuiInputTextFlags_CallbackAlways |
					                                        ImGuiInputTextFlags_EnterReturnsTrue;
					const bool submitted = ImGui::InputText(row_id.c_str(),
					                                       smart_editor_state.line_buffer.data(),
					                                       smart_editor_state.line_buffer.capacity() + 1,
					                                       input_flags,
					                                       editor_line_input_callback,
					                                       &callback_data);
					const bool line_edited = ImGui::IsItemEdited();
					const bool item_active = ImGui::IsItemActive();
					ImGui::PopStyleVar(2);
					ImGui::PopStyleColor(2);

					if (line_edited) {
						lines[static_cast<std::size_t>(line_index)] = smart_editor_state.line_buffer;
						commit_editor_lines(app_state, lines);
					}

					if (submitted) {
						const int cursor = std::clamp(smart_editor_state.cursor_column,
						                              0,
						                              static_cast<int>(smart_editor_state.line_buffer.size()));
						const std::string before = smart_editor_state.line_buffer.substr(0, static_cast<std::size_t>(cursor));
						const std::string after = smart_editor_state.line_buffer.substr(static_cast<std::size_t>(cursor));
						lines[static_cast<std::size_t>(line_index)] = before;
						lines.insert(lines.begin() + line_index + 1, after);
						commit_editor_lines(app_state, lines);
						smart_editor_state.cursor_column = 0;
						set_active_editor_line(lines, line_index + 1, true);
						continue;
					}

					if (item_active && smart_editor_state.selection_start == smart_editor_state.selection_end) {
						bool handled_special_key = false;
						if (ImGui::IsKeyPressed(ImGuiKey_Backspace) &&
						    smart_editor_state.cursor_column == 0 &&
						    line_index > 0) {
							const int previous_line = line_index - 1;
							const int previous_length = static_cast<int>(lines[static_cast<std::size_t>(previous_line)].size());
							lines[static_cast<std::size_t>(previous_line)] += smart_editor_state.line_buffer;
							lines.erase(lines.begin() + line_index);
							commit_editor_lines(app_state, lines);
							smart_editor_state.cursor_column = previous_length;
							set_active_editor_line(lines, previous_line, true);
							handled_special_key = true;
						}
						if (!handled_special_key &&
						    ImGui::IsKeyPressed(ImGuiKey_Delete) &&
						    smart_editor_state.cursor_column == static_cast<int>(smart_editor_state.line_buffer.size()) &&
						    line_index + 1 < static_cast<int>(lines.size())) {
							lines[static_cast<std::size_t>(line_index)] += lines[static_cast<std::size_t>(line_index + 1)];
							lines.erase(lines.begin() + line_index + 1);
							commit_editor_lines(app_state, lines);
							set_active_editor_line(lines, line_index, true);
							smart_editor_state.cursor_column =
								std::min(smart_editor_state.cursor_column,
								         static_cast<int>(smart_editor_state.line_buffer.size()));
							handled_special_key = true;
						}
						if (!handled_special_key && ImGui::IsKeyPressed(ImGuiKey_UpArrow) && line_index > 0) {
							lines[static_cast<std::size_t>(line_index)] = smart_editor_state.line_buffer;
							commit_editor_lines(app_state, lines);
							set_active_editor_line(lines, line_index - 1, true);
							handled_special_key = true;
						}
						if (!handled_special_key &&
						    ImGui::IsKeyPressed(ImGuiKey_DownArrow) &&
						    line_index + 1 < static_cast<int>(lines.size())) {
							lines[static_cast<std::size_t>(line_index)] = smart_editor_state.line_buffer;
							commit_editor_lines(app_state, lines);
							set_active_editor_line(lines, line_index + 1, true);
							handled_special_key = true;
						}
						if (!handled_special_key && ImGui::IsKeyPressed(ImGuiKey_Tab)) {
							smart_editor_state.line_buffer.insert(static_cast<std::size_t>(smart_editor_state.cursor_column), "    ");
							smart_editor_state.cursor_column += 4;
							lines[static_cast<std::size_t>(line_index)] = smart_editor_state.line_buffer;
							commit_editor_lines(app_state, lines);
							smart_editor_state.request_focus = true;
						}
					}
				} else {
					ImGui::InvisibleButton(row_id.c_str(), row_size);
					const bool row_hovered = ImGui::IsItemHovered();
					if (row_hovered) {
						draw_list->AddRectFilled(row_min,
						                         ImVec2(row_min.x + row_size.x, row_min.y + row_size.y),
						                         ImGui::ColorConvertFloat4ToU32(color_from_hex(0xf3ede5)));
					}
					const std::vector<EditorToken> tokens =
						tokenize_grammar_line(lines[static_cast<std::size_t>(line_index)], symbols);
					float text_x = row_min.x + 6.0f;
					EditorToken hovered_token;
					bool has_hovered_token = false;
					int hovered_token_column = -1;
					int token_column = 0;
					for (const EditorToken &token : tokens) {
						const ImVec2 token_size = ImGui::CalcTextSize(token.text.c_str());
						const int current_token_column = token_column;
						if (!token.text.empty()) {
							draw_list->AddText(ImVec2(text_x, row_min.y + 2.0f),
							                   color_for_editor_token(token.kind),
							                   token.text.c_str());
						}
						if (row_hovered && !token.tooltip.empty()) {
							const ImVec2 mouse = ImGui::GetIO().MousePos;
							if (mouse.x >= text_x && mouse.x <= text_x + token_size.x &&
							    mouse.y >= row_min.y && mouse.y <= row_min.y + row_size.y) {
								hovered_token = token;
								has_hovered_token = true;
								hovered_token_column = current_token_column;
							}
						}
						text_x += token_size.x;
						token_column += static_cast<int>(token.text.size());
					}
					if (row_hovered && ImGui::IsItemClicked()) {
						set_active_editor_line(lines, line_index, true);
					}
					if (has_hovered_token) {
						ImGui::BeginTooltip();
						ImGui::TextUnformatted(hovered_token.text.c_str());
						draw_divider();
						ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
						ImGui::TextWrapped("%s", hovered_token.tooltip.c_str());
						ImGui::PopTextWrapPos();
						ImGui::EndTooltip();
					}
					if (row_hovered && has_hovered_token && ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
						smart_editor_state.context_menu_has_token = true;
						smart_editor_state.context_menu_token = hovered_token;
						smart_editor_state.context_menu_line = line_index;
						smart_editor_state.context_menu_column = hovered_token_column;
						ImGui::OpenPopup("EditorTokenContextMenu");
					}
				}
			}
		}
		ImGui::EndTable();
	}

	if (ImGui::BeginPopup("EditorTokenContextMenu")) {
		if (smart_editor_state.context_menu_has_token) {
			if (ImGui::MenuItem("Find Next")) {
				const EditorTokenMatch next_match =
					find_next_token_match(lines,
					                      symbols,
					                      smart_editor_state.context_menu_token,
					                      smart_editor_state.context_menu_line,
					                      smart_editor_state.context_menu_column);
				if (next_match.found) {
					jump_to_editor_location(lines, next_match.line, next_match.column);
					debugout("Found next \"" + smart_editor_state.context_menu_token.text + "\" at line " +
					         std::to_string(next_match.line + 1));
				} else {
					debugout("No next match for \"" + smart_editor_state.context_menu_token.text + "\"");
				}
			}
		} else {
			ImGui::BeginDisabled();
			ImGui::MenuItem("Find Next", nullptr, false, false);
			ImGui::EndDisabled();
		}
		ImGui::EndPopup();
	}

	if (app_fonts.mono != nullptr) {
		ImGui::PopFont();
	}
	ImGui::EndChild();
}

void draw_editor(AppState *app_state)
{
	ImGui::TextUnformatted("Grammar Workspace");
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x7d8a98));
	ImGui::TextUnformatted("Edit grammars, manage file outputs, and curate texture bindings.");
	ImGui::PopStyleColor();
	draw_divider();

	if (ImGui::Button("Open")) {
		if (load_grammar_into_editor(app_state, grammar_filename)) {
			regenerate_scene(app_state->editor_text);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Save")) {
		if (write_text_file(grammar_filename, app_state->editor_text)) {
			debugout("Saved grammar: " + grammar_filename);
		} else {
			errorout("Failed to save grammar: " + grammar_filename);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Export PLY")) {
		std::filesystem::path export_path = std::filesystem::path(grammar_filename).replace_extension(".ply");
		export_ply(export_path.string());
	}
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, color_from_hex(0x2f8f46));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color_from_hex(0x39a654));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, color_from_hex(0x28763a));
	if (ImGui::Button("Run")) {
		regenerate_scene(app_state->editor_text);
	}
	ImGui::PopStyleColor(3);

	draw_divider();

	if (ImGui::BeginTabBar("WorkspaceTabs")) {
		if (ImGui::BeginTabItem("Editor")) {
			draw_smart_editor(app_state);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Textures")) {
			ImGui::BeginChild("TexturesTabBody", ImVec2(0.0f, 0.0f), false);
			draw_texture_controls(app_state);
			ImGui::EndChild();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}
}

void draw_main_layout(AppState *app_state, float delta_seconds)
{
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float header_height = 88.0f;
	const float splitter_thickness = 10.0f;

	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
	                                ImGuiWindowFlags_NoMove |
	                                ImGuiWindowFlags_NoSavedSettings;
	ImGui::Begin("Progen3d", nullptr, window_flags);

	begin_surface("HeaderSurface", ImVec2(0.0f, header_height));
	ImGui::BeginGroup();
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x7d8a98));
	ImGui::TextUnformatted("PROCEDURAL MODEL STUDIO");
	ImGui::PopStyleColor();
	if (app_fonts.title != nullptr) {
		ImGui::PushFont(app_fonts.title);
	}
	ImGui::TextUnformatted("Progen3d");
	if (app_fonts.title != nullptr) {
		ImGui::PopFont();
	}
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x566473));
	ImGui::TextWrapped("Grammar authoring, material curation, and live 3D inspection.");
	ImGui::PopStyleColor();
	ImGui::EndGroup();
	ImGui::SameLine();
	const int header_stat_columns = viewport->Size.x < 1180.0f ? 2 : 4;
	const float header_stat_width = header_stat_columns == 2 ? 420.0f : 620.0f;
	ImGui::SetCursorPosX(std::max(ImGui::GetCursorPosX(), ImGui::GetWindowWidth() - header_stat_width));
	if (ImGui::BeginTable("HeaderStats", header_stat_columns, ImGuiTableFlags_SizingStretchProp)) {
		if (header_stat_columns == 2) {
			ImGui::TableSetupColumn("PrimaryStat", ImGuiTableColumnFlags_WidthStretch, 1.35f);
			ImGui::TableSetupColumn("SecondaryStat", ImGuiTableColumnFlags_WidthStretch, 1.0f);
		} else {
			ImGui::TableSetupColumn("ActiveFileStat", ImGuiTableColumnFlags_WidthStretch, 2.3f);
			ImGui::TableSetupColumn("RulesStat", ImGuiTableColumnFlags_WidthStretch, 0.8f);
			ImGui::TableSetupColumn("TokensStat", ImGuiTableColumnFlags_WidthStretch, 0.8f);
			ImGui::TableSetupColumn("TexturesStat", ImGuiTableColumnFlags_WidthStretch, 0.8f);
		}
		ImGui::TableNextColumn();
		draw_stat_block("Active File", std::filesystem::path(grammar_filename).filename().string(), true, true);
		ImGui::TableNextColumn();
		draw_stat_block("Rules", std::to_string(grammar != nullptr ? grammar->rule_list.size() : 0U));
		ImGui::TableNextColumn();
		draw_stat_block("Tokens", std::to_string(grammar != nullptr ? grammar->tokens_new.size() : 0U));
		ImGui::TableNextColumn();
		draw_stat_block("Textures", std::to_string(texture_list.size()));
		ImGui::EndTable();
	}
	end_surface();

	ImGui::Dummy(ImVec2(0.0f, 8.0f));
	const ImVec2 content_size = ImGui::GetContentRegionAvail();
	const float total_width = std::max(content_size.x, 720.0f);
	const float total_height = std::max(content_size.y, 320.0f);
	const float min_workspace_width = 360.0f;
	const float min_right_width = 320.0f;
	const float min_preview_height = 220.0f;
	const float min_console_height = 140.0f;

	float workspace_width =
		total_width * std::clamp(app_state != nullptr ? app_state->workspace_width_ratio : 0.46f, 0.25f, 0.75f);
	workspace_width = std::clamp(workspace_width,
	                             min_workspace_width,
	                             total_width - min_right_width - splitter_thickness);
	float right_column_width = total_width - workspace_width - splitter_thickness;

	begin_surface("EditorRegion", ImVec2(workspace_width, total_height));
	draw_editor(app_state);
	end_surface();

	ImGui::SameLine();
	float preview_height =
		total_height * std::clamp(app_state != nullptr ? app_state->preview_height_ratio : 0.68f, 0.35f, 0.85f);
	float console_height = total_height - preview_height - splitter_thickness;
	preview_height = std::clamp(preview_height,
	                            min_preview_height,
	                            total_height - min_console_height - splitter_thickness);
	console_height = total_height - preview_height - splitter_thickness;

	draw_splitter("##workspace_splitter",
	              true,
	              splitter_thickness,
	              total_height,
	              &workspace_width,
	              &right_column_width,
	              min_workspace_width,
	              min_right_width);
	if (app_state != nullptr) {
		app_state->workspace_width_ratio = workspace_width / total_width;
	}

	ImGui::SameLine(0.0f, 0.0f);

	ImGui::BeginChild("RightColumn", ImVec2(0.0f, total_height), false, ImGuiWindowFlags_NoScrollbar);
	begin_surface("PreviewRegion", ImVec2(0.0f, preview_height));
	draw_preview(app_state, delta_seconds);
	end_surface();

	draw_splitter("##console_splitter",
	              false,
	              splitter_thickness,
	              std::max(ImGui::GetContentRegionAvail().x, 64.0f),
	              &preview_height,
	              &console_height,
	              min_preview_height,
	              min_console_height);
	if (app_state != nullptr) {
		app_state->preview_height_ratio = preview_height / total_height;
	}

	begin_surface("ConsoleRegion", ImVec2(0.0f, 0.0f));
	draw_console();
	end_surface();
	ImGui::EndChild();

	ImGui::End();
}

void glfw_error_callback(int, const char *description)
{
	errorout(std::string("GLFW error: ") + description);
}

}

void errorout(std::string error_str)
{
	append_console_message(error_str);
	std::cerr << error_str << '\n';
}

void debugout(const std::string &message)
{
	append_console_message(std::string("[DEBUG] ") + message);
	std::clog << message << '\n';
}

void debugstate(const std::string &key, const std::string &message)
{
	auto it = debug_state_messages.find(key);
	if (it != debug_state_messages.end() && it->second == message) {
		return;
	}
	debug_state_messages[key] = message;
	debugout(message);
}

void upload_fulltext(std::string)
{
}

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	const uint64_t time_seed =
		static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	std::seed_seq seed_seq{static_cast<uint32_t>(time_seed & 0xffffffffu),
	                       static_cast<uint32_t>(time_seed >> 32)};
	rng.seed(seed_seq);

	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit()) {
		errorout("Failed to initialize GLFW");
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow *window = glfwCreateWindow(1180, 760, "Progen3d", nullptr, nullptr);
	if (window == nullptr) {
		errorout("Failed to create GLFW window");
		glfwTerminate();
		return 1;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	if (!initialize_renderer()) {
		errorout("Failed to initialize renderer");
		glfwDestroyWindow(window);
		glfwTerminate();
		return 1;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	load_professional_fonts();
	apply_professional_style();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	AppState app_state;
	load_default_textures();

	if (!load_grammar_into_editor(&app_state, grammar_filename)) {
		app_state.editor_text = "X -> R Height ( 4 8 ) Y\nY -> S ( 1 Height 1 ) I ( Cube 0 0 1 )\n";
	}
	regenerate_scene(app_state.editor_text);

	double last_time = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		const double now = glfwGetTime();
		const float delta_seconds = static_cast<float>(now - last_time);
		last_time = now;

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		draw_main_layout(&app_state, delta_seconds);

		ImGui::Render();
		int display_w = 0;
		int display_h = 0;
		glfwGetFramebufferSize(window, &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.95f, 0.93f, 0.90f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	destroy_grammar();
	clear_variable_pool();
	destroy_textures();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	shutdown_renderer();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
