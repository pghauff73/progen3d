#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cfloat>
#include <cstdint>
#include <cstdarg>
#include <cmath>
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
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <epoxy/gl.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_internal.h>
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

enum class FileDialogAction {
	None,
	Open,
	SaveAs
};

struct FileDialogState {
	FileDialogAction action = FileDialogAction::None;
	std::string directory;
	std::string path_input;
	bool request_open = false;
	bool request_focus = false;
};

struct AppState {
	std::string editor_text;
	std::string texture_path;
	float workspace_width_ratio = 0.46f;
	float preview_height_ratio = 0.68f;
	bool editor_dirty = false;
	FileDialogState file_dialog;
};

struct AppFonts {
	ImFont *body = nullptr;
	ImFont *title = nullptr;
	ImFont *mono = nullptr;
};

struct UiImage {
	GLuint texture = 0;
	int width = 0;
	int height = 0;
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

struct EditorOverlayLayout {
	ImGuiWindow *window = nullptr;
	ImDrawList *draw_list = nullptr;
	ImRect clip_rect;
	ImVec2 content_origin;
	float line_height = 0.0f;
	float font_size = 0.0f;
};

struct GrammarSymbolIndex {
	std::unordered_set<std::string> rule_names;
	std::unordered_set<std::string> variable_names;
};

struct RegenerationResult {
	Grammar *grammar = nullptr;
	std::vector<std::vector<GLfloat>> vertex_buffers;
	std::vector<int> counts;
	unsigned long long texture_binding_version = 0;
	uint64_t request_id = 0;
	std::string error;
	std::string temp_filename;
};

struct SmartEditorState {
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
	std::string synced_text;
	EditorToken selected_identifier_token;
};

std::mutex log_mutex;
std::vector<std::string> console_messages;
std::unordered_map<std::string, std::string> debug_state_messages;
AppFonts app_fonts;
UiImage header_logo;
SmartEditorState smart_editor_state;
std::mutex texture_binding_mutex;
unsigned long long texture_binding_version = 0;
std::atomic<bool> regeneration_in_progress{false};
std::mutex completed_regeneration_mutex;
std::unique_ptr<RegenerationResult> completed_regeneration_result;
std::thread regeneration_worker;
uint64_t next_regeneration_request_id = 0;
uint64_t latest_requested_regeneration_id = 0;
uint64_t pending_regeneration_request_id = 0;
bool has_pending_regeneration = false;
std::string pending_regeneration_source;

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
	style.WindowPadding = ImVec2(13.0f, 11.0f);
	style.FramePadding = ImVec2(8.0f, 6.0f);
	style.CellPadding = ImVec2(6.0f, 5.0f);
	style.ItemSpacing = ImVec2(7.0f, 6.0f);
	style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
	style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
	style.IndentSpacing = 12.0f;
	style.ScrollbarSize = 10.0f;
	style.GrabMinSize = 8.0f;
	style.WindowBorderSize = 0.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;
	style.TabBorderSize = 0.0f;
	style.WindowRounding = 18.0f;
	style.ChildRounding = 16.0f;
	style.FrameRounding = 10.0f;
	style.PopupRounding = 12.0f;
	style.ScrollbarRounding = 12.0f;
	style.GrabRounding = 10.0f;
	style.TabRounding = 10.0f;

	ImVec4 *colors = style.Colors;
	colors[ImGuiCol_Text] = color_from_hex(0x3a3632);
	colors[ImGuiCol_TextDisabled] = color_from_hex(0x9b938c);
	colors[ImGuiCol_WindowBg] = color_from_hex(0xf7f3ed);
	colors[ImGuiCol_ChildBg] = color_from_hex(0xfffcf8);
	colors[ImGuiCol_PopupBg] = color_from_hex(0xfffcf9, 0.98f);
	colors[ImGuiCol_Border] = color_from_hex(0xe0d8d1, 0.95f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	colors[ImGuiCol_FrameBg] = color_from_hex(0xf3ede6);
	colors[ImGuiCol_FrameBgHovered] = color_from_hex(0xefe4ee);
	colors[ImGuiCol_FrameBgActive] = color_from_hex(0xe8dceb);
	colors[ImGuiCol_TitleBg] = color_from_hex(0xfffcf8);
	colors[ImGuiCol_TitleBgActive] = color_from_hex(0xfffcf8);
	colors[ImGuiCol_TitleBgCollapsed] = color_from_hex(0xfffcf8);
	colors[ImGuiCol_MenuBarBg] = color_from_hex(0xfffcf8);
	colors[ImGuiCol_ScrollbarBg] = color_from_hex(0xf0eae3);
	colors[ImGuiCol_ScrollbarGrab] = color_from_hex(0xd8cfe0);
	colors[ImGuiCol_ScrollbarGrabHovered] = color_from_hex(0xc4bad6);
	colors[ImGuiCol_ScrollbarGrabActive] = color_from_hex(0xb0a4cb);
	colors[ImGuiCol_CheckMark] = color_from_hex(0x9bc5df);
	colors[ImGuiCol_SliderGrab] = color_from_hex(0x9bc5df);
	colors[ImGuiCol_SliderGrabActive] = color_from_hex(0xbba7d9);
	colors[ImGuiCol_Button] = color_from_hex(0xdbe8f4);
	colors[ImGuiCol_ButtonHovered] = color_from_hex(0xcfe0ef);
	colors[ImGuiCol_ButtonActive] = color_from_hex(0xc1d7ea);
	colors[ImGuiCol_Header] = color_from_hex(0xf1e6ef);
	colors[ImGuiCol_HeaderHovered] = color_from_hex(0xebdde9);
	colors[ImGuiCol_HeaderActive] = color_from_hex(0xe4d2e2);
	colors[ImGuiCol_Separator] = color_from_hex(0xe3dbd4);
	colors[ImGuiCol_SeparatorHovered] = color_from_hex(0xd5c7df);
	colors[ImGuiCol_SeparatorActive] = color_from_hex(0xc8b7d9);
	colors[ImGuiCol_ResizeGrip] = color_from_hex(0xd8cfdf, 0.45f);
	colors[ImGuiCol_ResizeGripHovered] = color_from_hex(0xd2c2df, 0.75f);
	colors[ImGuiCol_ResizeGripActive] = color_from_hex(0xc4b3d8, 0.9f);
	colors[ImGuiCol_Tab] = color_from_hex(0xf3ece6);
	colors[ImGuiCol_TabHovered] = color_from_hex(0xede1eb);
	colors[ImGuiCol_TabActive] = color_from_hex(0xfffcf8);
	colors[ImGuiCol_TabUnfocused] = color_from_hex(0xf2ebe4);
	colors[ImGuiCol_TabUnfocusedActive] = color_from_hex(0xf8f4ef);
	colors[ImGuiCol_PlotLines] = color_from_hex(0x9bc5df);
	colors[ImGuiCol_PlotLinesHovered] = color_from_hex(0x84b4d4);
	colors[ImGuiCol_PlotHistogram] = color_from_hex(0xe5b9a5);
	colors[ImGuiCol_PlotHistogramHovered] = color_from_hex(0xdca893);
	colors[ImGuiCol_TableHeaderBg] = color_from_hex(0xf3e7ee);
	colors[ImGuiCol_TableBorderStrong] = color_from_hex(0xe0d8df);
	colors[ImGuiCol_TableBorderLight] = color_from_hex(0xebe3e8);
	colors[ImGuiCol_TableRowBg] = color_from_hex(0xfffcf8, 0.0f);
	colors[ImGuiCol_TableRowBgAlt] = color_from_hex(0xfaf4f7, 0.45f);
	colors[ImGuiCol_TextSelectedBg] = color_from_hex(0xcfe3f1, 0.45f);
	colors[ImGuiCol_DragDropTarget] = color_from_hex(0xd5b9df, 0.95f);
	colors[ImGuiCol_NavHighlight] = color_from_hex(0xb2d2e8, 0.8f);
	colors[ImGuiCol_NavWindowingHighlight] = color_from_hex(0xb2d2e8, 0.8f);
	colors[ImGuiCol_NavWindowingDimBg] = color_from_hex(0x5f5a55, 0.12f);
	colors[ImGuiCol_ModalWindowDimBg] = color_from_hex(0x5f5a55, 0.20f);
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

void begin_surface(const char *id, const ImVec2 &size, bool border = true, ImVec2 padding = ImVec2(11.0f, 11.0f))
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, color_from_hex(0xfffcf8));
	ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xe0d8d1));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 14.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
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
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
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
	ImGui::PushStyleColor(ImGuiCol_Separator, color_from_hex(0xe3dbd4));
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
	ImGui::PushStyleColor(ImGuiCol_Button, color_from_hex(0xf1ebe5));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color_from_hex(0xeae2ef));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, color_from_hex(0xe1d7e7));
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

void clear_selected_identifier()
{
	smart_editor_state.has_selected_identifier = false;
	smart_editor_state.selected_identifier_line = -1;
	smart_editor_state.selected_identifier_column = -1;
	smart_editor_state.selected_identifier_token = {};
}

void reset_editor_interaction_state()
{
	smart_editor_state = SmartEditorState{};
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

void destroy_ui_image(UiImage *image)
{
	if (image == nullptr || image->texture == 0) {
		return;
	}

	glDeleteTextures(1, &image->texture);
	image->texture = 0;
	image->width = 0;
	image->height = 0;
}

std::string default_dialog_directory()
{
	std::error_code error;
	const std::filesystem::path cwd = std::filesystem::current_path(error);
	if (error) {
		return ".";
	}
	return cwd.lexically_normal().string();
}

std::string normalize_document_path(const std::string &path)
{
	if (path.empty()) {
		return "";
	}

	std::error_code error;
	std::filesystem::path candidate(path);
	if (candidate.is_relative()) {
		candidate = std::filesystem::absolute(candidate, error);
		if (error) {
			error.clear();
			candidate = std::filesystem::path(default_dialog_directory()) / path;
		}
	}
	return candidate.lexically_normal().string();
}

std::string parent_directory_for_path(const std::string &path)
{
	if (path.empty()) {
		return default_dialog_directory();
	}

	const std::filesystem::path candidate(path);
	const std::filesystem::path parent = candidate.has_parent_path() ? candidate.parent_path() : std::filesystem::path(default_dialog_directory());
	return parent.empty() ? default_dialog_directory() : parent.lexically_normal().string();
}

std::string display_document_name(const AppState *app_state)
{
	std::string label = grammar_filename.empty()
	                        ? "Untitled"
	                        : std::filesystem::path(grammar_filename).filename().string();
	if (app_state != nullptr && app_state->editor_dirty) {
		label += " *";
	}
	return label;
}

std::string default_new_document_text()
{
	return "X -> R Height ( 4 8 ) Y\n"
	       "Y -> S ( 1 Height 1 ) I ( Cube 0 0 1 )\n";
}

void note_texture_binding_changed(const std::string &reason)
{
	unsigned long long version = 0;
	{
		std::lock_guard<std::mutex> lock(texture_binding_mutex);
		version = ++texture_binding_version;
	}
	debugout("Texture bindings updated: " + reason + ", version=" + std::to_string(version));
}

std::vector<int> snapshot_texture_bindings(unsigned long long *version_out = nullptr)
{
	std::lock_guard<std::mutex> lock(texture_binding_mutex);
	if (version_out != nullptr) {
		*version_out = texture_binding_version;
	}
	return texture_list;
}

unsigned long long current_texture_binding_version()
{
	std::lock_guard<std::mutex> lock(texture_binding_mutex);
	return texture_binding_version;
}

void join_regeneration_worker_if_idle()
{
	if (regeneration_worker.joinable() && !regeneration_in_progress.load()) {
		regeneration_worker.join();
	}
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

std::vector<std::string> breakup_into_lines(std::string input, std::string delimiter);
bool input_text_string(const char *label, std::string *text, ImGuiInputTextFlags flags = 0);

void start_regeneration_worker(uint64_t request_id, const std::string &source_text)
{
	join_regeneration_worker_if_idle();

	unsigned long long binding_version_snapshot = 0;
	std::vector<int> texture_list_snapshot = snapshot_texture_bindings(&binding_version_snapshot);
	regeneration_in_progress = true;

	regeneration_worker = std::thread([request_id,
	                                   source_text,
	                                   binding_version_snapshot,
	                                   texture_list_snapshot = std::move(texture_list_snapshot)]() mutable {
		std::unique_ptr<RegenerationResult> result(new RegenerationResult());
		result->request_id = request_id;
		result->texture_binding_version = binding_version_snapshot;

		try {
			const std::filesystem::path temp_path =
				std::filesystem::temp_directory_path() /
				("progen3d-imgui-current-" + std::to_string(request_id) + ".grammar");
			result->temp_filename = temp_path.string();
			if (!write_text_file(result->temp_filename, source_text)) {
				result->error = "Failed to write temporary grammar file";
			} else {
				clear_variable_pool();
				std::unique_ptr<Grammar> next_grammar(new Grammar(result->temp_filename));
				next_grammar->lines = breakup_into_lines(source_text, "\n");
				if (!next_grammar->lines.empty()) {
					next_grammar->Reread();
				}
				next_grammar->addContext();
				for (int texture : texture_list_snapshot) {
					next_grammar->context->loadTexture(static_cast<GLuint>(texture));
				}
				next_grammar->context->genPrimitives();
				next_grammar->generateGeometry();

				auto base_vertices = build_base_vertex_data();
				next_grammar->context->buildTextureBuffers(base_vertices.data(),
				                                          texture_list_snapshot.size(),
				                                          &result->vertex_buffers,
				                                          &result->counts);
				result->grammar = next_grammar.release();
			}
		}
		catch (const std::exception &exception) {
			result->error = std::string("Regeneration failed: ") + exception.what();
		}
		catch (...) {
			result->error = "Regeneration failed";
		}

		{
			std::lock_guard<std::mutex> lock(completed_regeneration_mutex);
			completed_regeneration_result = std::move(result);
		}
		regeneration_in_progress = false;
	});
}

void request_scene_regeneration(const std::string &source_text)
{
	const uint64_t request_id = ++next_regeneration_request_id;
	latest_requested_regeneration_id = request_id;

	if (regeneration_in_progress.load()) {
		has_pending_regeneration = true;
		pending_regeneration_request_id = request_id;
		pending_regeneration_source = source_text;
		return;
	}

	start_regeneration_worker(request_id, source_text);
}

void process_completed_regeneration()
{
	std::unique_ptr<RegenerationResult> result;
	{
		std::lock_guard<std::mutex> lock(completed_regeneration_mutex);
		result = std::move(completed_regeneration_result);
	}
	if (result == nullptr) {
		return;
	}

	join_regeneration_worker_if_idle();

	if (!result->temp_filename.empty()) {
		std::error_code error;
		std::filesystem::remove(result->temp_filename, error);
	}

	bool applied_result = false;
	if (result->request_id != latest_requested_regeneration_id) {
		debugout("Discarded stale regeneration result");
	} else if (!result->error.empty() || result->grammar == nullptr) {
		errorout(result->error.empty() ? "Regeneration failed" : result->error);
	} else if (current_texture_binding_version() != result->texture_binding_version) {
		debugout("Discarded stale regeneration result because texture bindings changed");
	} else {
		destroy_grammar();
		grammar = result->grammar;
		result->grammar = nullptr;
		upload_render_buffers(result->vertex_buffers, result->counts, texture_list.size());
		debugout("Regenerated scene");
		applied_result = true;
	}

	if (result->grammar != nullptr) {
		delete result->grammar;
		result->grammar = nullptr;
	}

	if (has_pending_regeneration) {
		const uint64_t pending_request_id = pending_regeneration_request_id;
		const std::string pending_source = pending_regeneration_source;
		has_pending_regeneration = false;
		pending_regeneration_source.clear();
		start_regeneration_worker(pending_request_id, pending_source);
	} else if (applied_result) {
		join_regeneration_worker_if_idle();
	}
}

void set_editor_document(AppState *app_state,
                         const std::string &text,
                         const std::string &path,
                         bool dirty)
{
	if (app_state == nullptr) {
		return;
	}

	app_state->editor_text = text;
	app_state->editor_dirty = dirty;
	grammar_filename = path;
	reset_editor_interaction_state();

	if (app_state->file_dialog.directory.empty()) {
		app_state->file_dialog.directory = default_dialog_directory();
	}
	if (!grammar_filename.empty()) {
		app_state->file_dialog.directory = parent_directory_for_path(grammar_filename);
		app_state->file_dialog.path_input = grammar_filename;
	} else {
		app_state->file_dialog.path_input =
			normalize_document_path(app_state->file_dialog.directory + "/untitled.grammar");
	}
}

std::string ensure_grammar_extension(std::string path)
{
	if (path.empty()) {
		return path;
	}
	if (std::filesystem::path(path).extension().empty()) {
		path += ".grammar";
	}
	return path;
}

bool save_document_to_path(AppState *app_state, const std::string &requested_path)
{
	if (app_state == nullptr) {
		return false;
	}

	std::string path = ensure_grammar_extension(normalize_document_path(requested_path));
	if (path.empty()) {
		errorout("Document path is empty");
		return false;
	}
	if (!write_text_file(path, app_state->editor_text)) {
		errorout("Failed to save grammar: " + path);
		return false;
	}

	grammar_filename = path;
	app_state->editor_dirty = false;
	app_state->file_dialog.directory = parent_directory_for_path(grammar_filename);
	app_state->file_dialog.path_input = grammar_filename;
	debugout("Saved grammar: " + grammar_filename);
	return true;
}

bool open_document_from_path(AppState *app_state, const std::string &requested_path)
{
	if (app_state == nullptr) {
		return false;
	}

	const std::string normalized_path = normalize_document_path(requested_path);
	std::string text;
	if (!read_text_file(normalized_path, &text)) {
		errorout("Could not open grammar file: " + normalized_path);
		return false;
	}

	set_editor_document(app_state, text, normalized_path, false);
	debugout("Loaded grammar: " + normalized_path);
	request_scene_regeneration(app_state->editor_text);
	return true;
}

void create_new_document(AppState *app_state)
{
	if (app_state == nullptr) {
		return;
	}

	set_editor_document(app_state, default_new_document_text(), "", false);
	debugout("Created new grammar document");
	request_scene_regeneration(app_state->editor_text);
}

void open_file_dialog(AppState *app_state, FileDialogAction action)
{
	if (app_state == nullptr) {
		return;
	}

	FileDialogState &state = app_state->file_dialog;
	state.action = action;
	state.request_open = true;
	state.request_focus = true;
	state.directory = !state.directory.empty() ? state.directory : default_dialog_directory();

	if (action == FileDialogAction::Open) {
		state.path_input = !grammar_filename.empty() ? grammar_filename : state.directory;
	} else if (action == FileDialogAction::SaveAs) {
		if (!grammar_filename.empty()) {
			state.path_input = grammar_filename;
		} else {
			state.path_input = normalize_document_path(state.directory + "/untitled.grammar");
		}
	}
}

struct FileDialogEntry {
	std::string label;
	std::string path;
	bool is_directory = false;
};

std::vector<FileDialogEntry> list_file_dialog_entries(const std::string &directory)
{
	std::vector<FileDialogEntry> entries;
	std::error_code error;
	const std::filesystem::path root(directory.empty() ? default_dialog_directory() : directory);
	if (!std::filesystem::exists(root, error) || error) {
		return entries;
	}

	for (std::filesystem::directory_iterator it(root, error); !error && it != std::filesystem::directory_iterator(); it.increment(error)) {
		const std::filesystem::directory_entry &entry = *it;
		std::error_code status_error;
		const bool is_directory = entry.is_directory(status_error);
		if (status_error) {
			continue;
		}
		if (!is_directory) {
			const std::string extension = entry.path().extension().string();
			if (extension != ".grammar") {
				continue;
			}
		}

		FileDialogEntry dialog_entry;
		dialog_entry.is_directory = is_directory;
		dialog_entry.path = entry.path().lexically_normal().string();
		dialog_entry.label = (is_directory ? "[DIR] " : "") + entry.path().filename().string();
		entries.push_back(std::move(dialog_entry));
	}

	std::sort(entries.begin(), entries.end(), [](const FileDialogEntry &lhs, const FileDialogEntry &rhs) {
		if (lhs.is_directory != rhs.is_directory) {
			return lhs.is_directory > rhs.is_directory;
		}
		return lhs.label < rhs.label;
	});
	return entries;
}

const char *file_dialog_title(FileDialogAction action)
{
	switch (action) {
	case FileDialogAction::Open:
		return "Open Grammar";
	case FileDialogAction::SaveAs:
		return "Save Grammar As";
	case FileDialogAction::None:
	default:
		return "Grammar File";
	}
}

const char *file_dialog_confirm_label(FileDialogAction action)
{
	switch (action) {
	case FileDialogAction::Open:
		return "Open";
	case FileDialogAction::SaveAs:
		return "Save";
	case FileDialogAction::None:
	default:
		return "OK";
	}
}

void draw_file_dialog(AppState *app_state)
{
	if (app_state == nullptr) {
		return;
	}

	FileDialogState &state = app_state->file_dialog;
	if (state.action == FileDialogAction::None) {
		return;
	}

	const char *title = file_dialog_title(state.action);
	if (state.request_open) {
		ImGui::OpenPopup(title);
		state.request_open = false;
	}

	if (!ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		return;
	}

	ImGui::TextUnformatted("Directory");
	input_text_string("##file_dialog_directory", &state.directory);
	if (ImGui::Button("Browse")) {
		const std::string normalized = normalize_document_path(state.directory);
		if (!normalized.empty()) {
			state.directory = normalized;
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Up")) {
		state.directory = parent_directory_for_path(state.directory);
	}

	ImGui::TextUnformatted("Path");
	if (state.request_focus) {
		ImGui::SetKeyboardFocusHere();
		state.request_focus = false;
	}
	input_text_string("##file_dialog_path", &state.path_input);

	ImGui::BeginChild("##file_dialog_entries", ImVec2(560.0f, 260.0f), true);
	for (const FileDialogEntry &entry : list_file_dialog_entries(state.directory)) {
		const bool selected = normalize_document_path(state.path_input) == entry.path;
		if (ImGui::Selectable(entry.label.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
			if (entry.is_directory) {
				state.directory = entry.path;
				if (state.action == FileDialogAction::SaveAs) {
					const std::string filename =
						std::filesystem::path(state.path_input).filename().empty()
							? "untitled.grammar"
							: std::filesystem::path(state.path_input).filename().string();
					state.path_input = normalize_document_path(entry.path + "/" + filename);
				} else {
					state.path_input = entry.path;
				}
			} else {
				state.path_input = entry.path;
				if (state.action == FileDialogAction::Open && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					if (open_document_from_path(app_state, state.path_input)) {
						state.action = FileDialogAction::None;
						ImGui::CloseCurrentPopup();
					}
				}
			}
		}
	}
	ImGui::EndChild();

	bool close_dialog = false;
	if (ImGui::Button(file_dialog_confirm_label(state.action))) {
		std::filesystem::path requested_path(state.path_input);
		std::error_code error;
		if (std::filesystem::exists(requested_path, error) && std::filesystem::is_directory(requested_path, error)) {
			state.directory = normalize_document_path(requested_path.string());
			if (state.action == FileDialogAction::SaveAs) {
				state.path_input = normalize_document_path(state.directory + "/untitled.grammar");
			}
		} else if (state.action == FileDialogAction::Open) {
			close_dialog = open_document_from_path(app_state, state.path_input);
		} else if (state.action == FileDialogAction::SaveAs) {
			close_dialog = save_document_to_path(app_state, state.path_input);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel")) {
		close_dialog = true;
	}

	if (close_dialog) {
		state.action = FileDialogAction::None;
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
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

std::string trim_copy(const std::string &input)
{
	const std::size_t start = input.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) {
		return "";
	}
	const std::size_t end = input.find_last_not_of(" \t\r\n");
	return input.substr(start, end - start + 1);
}

bool is_blank_or_comment_line(const std::string &line)
{
	const std::string trimmed = trim_copy(line);
	return trimmed.empty() || trimmed[0] == '#';
}

bool starts_new_rule(const std::string &trimmed_line)
{
	if (trimmed_line.empty()) {
		return false;
	}
	if (trimmed_line.rfind("->", 0) == 0) {
		return false;
	}
	return trimmed_line.find("->") != std::string::npos;
}

int first_non_whitespace_column(const std::string &line)
{
	const std::size_t start = line.find_first_not_of(" \t\r\n");
	return start == std::string::npos ? 0 : static_cast<int>(start);
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
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0x9ba2b2));
	case EditorTokenKind::RuleDefinition:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0x6b9b86));
	case EditorTokenKind::RuleReference:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0x789cc7));
	case EditorTokenKind::Variable:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0xcc9b76));
	case EditorTokenKind::Number:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0xc98787));
	case EditorTokenKind::Operator:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0xa890c8));
	case EditorTokenKind::Keyword:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0x7195bb));
	case EditorTokenKind::Primitive:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0xc58cb0));
	case EditorTokenKind::Identifier:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0xb59b82));
	case EditorTokenKind::Plain:
	default:
		return ImGui::GetColorU32(ImGuiCol_Text);
	}
}

bool is_expression_operator_token(const std::string &token)
{
	return token == "+" || token == "-" || token == "*" || token == "/";
}

ImU32 color_for_editor_token(const EditorToken &token)
{
	if (token.kind == EditorTokenKind::Number) {
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0xc98787));
	}
	if (token.kind == EditorTokenKind::Operator && is_expression_operator_token(token.text)) {
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0xc98787));
	}
	return color_for_editor_token(token.kind);
}

ImU32 color_for_diagnostic(EditorDiagnosticSeverity severity)
{
	switch (severity) {
	case EditorDiagnosticSeverity::Warning:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0xc6a06d));
	case EditorDiagnosticSeverity::Error:
	default:
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0xca7f7f));
	}
}

ImU32 color_for_identifier_match_fill(bool active)
{
	return ImGui::ColorConvertFloat4ToU32(
		active ? color_from_hex(0xf0ddaf, 0.35f) : color_from_hex(0xe6dff1, 0.26f));
}

ImU32 color_for_bracket_match_fill(bool active, bool matched)
{
	if (!matched) {
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0xe7c2c2, 0.35f));
	}
	return ImGui::ColorConvertFloat4ToU32(
		active ? color_from_hex(0xcfe5d7, 0.34f) : color_from_hex(0xdedaf0, 0.24f));
}

bool is_opening_delimiter(char c)
{
	return c == '(' || c == '[' || c == '{';
}

bool is_closing_delimiter(char c)
{
	return c == ')' || c == ']' || c == '}';
}

char matching_delimiter(char c)
{
	switch (c) {
	case '(':
		return ')';
	case '[':
		return ']';
	case '{':
		return '}';
	case ')':
		return '(';
	case ']':
		return '[';
	case '}':
		return '{';
	default:
		return '\0';
	}
}

bool delimiters_match(char open, char close)
{
	return (open == '(' && close == ')') ||
	       (open == '[' && close == ']') ||
	       (open == '{' && close == '}');
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

bool is_matchable_identifier_token(EditorTokenKind kind)
{
	return kind == EditorTokenKind::Identifier ||
	       kind == EditorTokenKind::Variable ||
	       kind == EditorTokenKind::RuleDefinition ||
	       kind == EditorTokenKind::RuleReference;
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

struct EditorTokenSpan {
	EditorToken token;
	int start_column = 0;
	int end_column = 0;
};

std::vector<EditorTokenSpan> tokenize_grammar_line_with_spans(const std::string &line,
                                                              const GrammarSymbolIndex &symbols)
{
	std::vector<EditorTokenSpan> spans;
	const std::vector<EditorToken> tokens = tokenize_grammar_line(line, symbols);
	spans.reserve(tokens.size());
	int column = 0;
	for (const EditorToken &token : tokens) {
		const int start_column = column;
		column += static_cast<int>(token.text.size());
		spans.push_back({token, start_column, column});
	}
	return spans;
}

EditorTokenMatch position_for_document_offset(const std::vector<std::string> &lines, int offset);

bool build_editor_overlay_layout(ImGuiID editor_id,
                                 const ImRect &editor_rect,
                                 EditorOverlayLayout *layout)
{
	if (layout == nullptr || editor_id == 0) {
		return false;
	}

	ImGuiWindow *editor_window = ImGui::FindWindowByID(editor_id);
	if (editor_window != nullptr) {
		const bool plausible_match =
			editor_window->ChildId == editor_id || editor_window->Rect().Overlaps(editor_rect);
		if (!plausible_match) {
			editor_window = nullptr;
		}
	}
	if (editor_window == nullptr) {
		ImGuiWindow *parent_window = ImGui::GetCurrentWindow();
		if (parent_window != nullptr) {
			for (int index = parent_window->DC.ChildWindows.Size - 1; index >= 0; --index) {
				ImGuiWindow *candidate = parent_window->DC.ChildWindows[index];
				if (candidate == nullptr) {
					continue;
				}
				const bool child_id_matches = candidate->ChildId == editor_id;
				const bool overlaps_item_rect = candidate->Rect().Overlaps(editor_rect);
				if (child_id_matches || overlaps_item_rect) {
					editor_window = candidate;
					break;
				}
			}
		}
	}
	if (editor_window == nullptr || editor_window->DrawList == nullptr) {
		return false;
	}

	layout->window = editor_window;
	layout->draw_list = editor_window->DrawList;
	layout->clip_rect = editor_window->InnerClipRect;
	layout->content_origin = editor_window->DC.CursorStartPos;
	if (ImGuiInputTextState *input_state = ImGui::GetInputTextState(editor_id)) {
		layout->content_origin.x -= input_state->ScrollX;
	}
	layout->line_height = ImGui::GetFontSize();
	layout->font_size = ImGui::GetFontSize();
	return layout->line_height > 0.0f;
}

bool editor_range_equals(const EditorRange &lhs, const EditorRange &rhs)
{
	return lhs.found == rhs.found &&
	       lhs.line == rhs.line &&
	       lhs.start_column == rhs.start_column &&
	       lhs.end_column == rhs.end_column;
}

ImVec2 editor_screen_position_for_column(const EditorOverlayLayout &layout,
                                         const std::string &line,
                                         int line_index,
                                         int column)
{
	column = std::clamp(column, 0, static_cast<int>(line.size()));
	const char *line_start = line.c_str();
	const char *line_end = line_start + column;
	const float x_offset = ImGui::CalcTextSize(line_start, line_end, false, -1.0f).x;
	return ImVec2(layout.content_origin.x + x_offset,
	              layout.content_origin.y + line_index * layout.line_height);
}

ImRect editor_rect_for_range(const EditorOverlayLayout &layout,
                             const std::vector<std::string> &lines,
                             const EditorRange &range)
{
	if (!range.found || range.line < 0 || range.line >= static_cast<int>(lines.size())) {
		return ImRect();
	}

	const std::string &line = lines[static_cast<std::size_t>(range.line)];
	const int start_column = std::clamp(range.start_column, 0, static_cast<int>(line.size()));
	int end_column = std::clamp(range.end_column, 0, static_cast<int>(line.size()));
	if (end_column <= start_column) {
		end_column = std::min(start_column + 1, static_cast<int>(line.size()));
	}

	const ImVec2 min = editor_screen_position_for_column(layout, line, range.line, start_column);
	ImVec2 max = editor_screen_position_for_column(layout, line, range.line, end_column);
	if (max.x <= min.x) {
		max.x = min.x + std::max(1.0f, layout.font_size * 0.55f);
	}
	max.y = min.y + layout.line_height;
	return ImRect(min, max);
}

bool editor_rect_for_token_range(const EditorOverlayLayout &layout,
                                 const std::vector<std::string> &lines,
                                 const GrammarSymbolIndex &symbols,
                                 const EditorRange &range,
                                 ImRect *rect_out)
{
	if (rect_out == nullptr || !range.found || range.line < 0 ||
	    range.line >= static_cast<int>(lines.size())) {
		return false;
	}

	const std::string &line = lines[static_cast<std::size_t>(range.line)];
	const std::vector<EditorTokenSpan> spans =
		tokenize_grammar_line_with_spans(line, symbols);
	ImVec2 token_pos(layout.content_origin.x,
	                 layout.content_origin.y + range.line * layout.line_height);
	for (const EditorTokenSpan &span : spans) {
		const float token_width =
			ImGui::CalcTextSize(span.token.text.c_str(), nullptr, false, -1.0f).x;
		if (span.start_column == range.start_column && span.end_column == range.end_column) {
			const float width = std::max(token_width, std::max(1.0f, layout.font_size * 0.55f));
			*rect_out = ImRect(token_pos,
			                   ImVec2(token_pos.x + width, token_pos.y + layout.line_height));
			return true;
		}
		token_pos.x += token_width;
	}

	return false;
}

void draw_editor_diagnostic_underline(const EditorOverlayLayout &layout,
                                      const std::vector<std::string> &lines,
                                      const EditorDiagnostic &diagnostic)
{
	const ImRect rect = editor_rect_for_range(layout, lines, diagnostic.range);
	if (rect.Max.x <= rect.Min.x || rect.Max.y <= rect.Min.y) {
		return;
	}

	const ImU32 color = color_for_diagnostic(diagnostic.severity);
	const float baseline = rect.Max.y - 2.0f;
	const float amplitude = 2.0f;
	const float step = 4.0f;
	float x = rect.Min.x;
	bool rising = true;
	while (x < rect.Max.x) {
		const float next_x = std::min(x + step, rect.Max.x);
		const float y1 = rising ? baseline : baseline - amplitude;
		const float y2 = rising ? baseline - amplitude : baseline;
		layout.draw_list->AddLine(ImVec2(x, y1), ImVec2(next_x, y2), color, 1.5f);
		x = next_x;
		rising = !rising;
	}
}

std::vector<EditorDiagnostic> build_editor_diagnostics(const std::vector<std::string> &lines)
{
	struct DelimiterState {
		char delimiter = '\0';
		int line = -1;
		int column = 0;
	};

	std::vector<EditorDiagnostic> diagnostics;
	std::vector<DelimiterState> delimiter_stack;
	int rule_block_start_line = -1;
	bool rule_block_has_arrow = false;

	const auto push_diagnostic = [&](int line,
	                                 int start_column,
	                                 int end_column,
	                                 EditorDiagnosticSeverity severity,
	                                 std::string message) {
		EditorDiagnostic diagnostic;
		diagnostic.range.line = line;
		diagnostic.range.start_column = std::max(0, start_column);
		diagnostic.range.end_column = std::max(diagnostic.range.start_column + 1, end_column);
		diagnostic.range.found = line >= 0;
		diagnostic.severity = severity;
		diagnostic.message = std::move(message);
		diagnostics.push_back(std::move(diagnostic));
	};

	const auto finalize_rule_block = [&]() {
		if (rule_block_start_line < 0 || rule_block_start_line >= static_cast<int>(lines.size())) {
			rule_block_start_line = -1;
			rule_block_has_arrow = false;
			return;
		}
		if (!rule_block_has_arrow) {
			const std::string &line = lines[static_cast<std::size_t>(rule_block_start_line)];
			const std::size_t comment_start = find_comment_start(line);
			const int start_column = first_non_whitespace_column(line);
			const int end_column = static_cast<int>(
				comment_start == std::string::npos ? line.size() : comment_start);
			push_diagnostic(rule_block_start_line,
			                start_column,
			                std::max(start_column + 1, end_column),
			                EditorDiagnosticSeverity::Error,
			                "Rule block is missing a '->' production arrow.");
		}
		rule_block_start_line = -1;
		rule_block_has_arrow = false;
	};

	for (int line_index = 0; line_index < static_cast<int>(lines.size()); ++line_index) {
		const std::string &line = lines[static_cast<std::size_t>(line_index)];
		if (!is_blank_or_comment_line(line)) {
			const std::string trimmed = trim_copy(line);
			if (rule_block_start_line < 0) {
				rule_block_start_line = line_index;
				rule_block_has_arrow = trimmed.find("->") != std::string::npos;
				if (trimmed.rfind("->", 0) == 0) {
					const int start_column = first_non_whitespace_column(line);
					push_diagnostic(line_index,
					                start_column,
					                start_column + 2,
					                EditorDiagnosticSeverity::Error,
					                "Production arrow needs a rule name on the left-hand side.");
				}
			} else if (starts_new_rule(trimmed)) {
				finalize_rule_block();
				rule_block_start_line = line_index;
				rule_block_has_arrow = true;
			} else if (trimmed.find("->") != std::string::npos) {
				rule_block_has_arrow = true;
			}
		}

		const std::size_t comment_start = find_comment_start(line);
		const std::size_t code_end = comment_start == std::string::npos ? line.size() : comment_start;
		for (std::size_t column = 0; column < code_end; ++column) {
			const char c = line[column];
			if (is_opening_delimiter(c)) {
				delimiter_stack.push_back({c, line_index, static_cast<int>(column)});
				continue;
			}
			if (!is_closing_delimiter(c)) {
				continue;
			}
			if (delimiter_stack.empty()) {
				push_diagnostic(line_index,
				                static_cast<int>(column),
				                static_cast<int>(column) + 1,
				                EditorDiagnosticSeverity::Error,
				                std::string("Unmatched closing delimiter '") + c + "'.");
				continue;
			}
			const DelimiterState &open = delimiter_stack.back();
			if (delimiters_match(open.delimiter, c)) {
				delimiter_stack.pop_back();
				continue;
			}
			push_diagnostic(line_index,
			                static_cast<int>(column),
			                static_cast<int>(column) + 1,
			                EditorDiagnosticSeverity::Error,
			                std::string("Mismatched closing delimiter '") + c +
			                    "'. Expected '" + matching_delimiter(open.delimiter) + "'.");
		}
	}

	finalize_rule_block();

	for (const DelimiterState &open : delimiter_stack) {
		push_diagnostic(open.line,
		                open.column,
		                open.column + 1,
		                EditorDiagnosticSeverity::Error,
		                std::string("Unclosed delimiter '") + open.delimiter + "'.");
	}

	return diagnostics;
}

bool find_matchable_identifier_token_near_cursor(const std::vector<std::string> &lines,
                                                 const GrammarSymbolIndex &symbols,
                                                 const EditorTokenMatch &cursor_position,
                                                 EditorToken *token_out,
                                                 EditorRange *range_out)
{
	if (!cursor_position.found || cursor_position.line < 0 ||
	    cursor_position.line >= static_cast<int>(lines.size())) {
		return false;
	}

	const std::string &line = lines[static_cast<std::size_t>(cursor_position.line)];
	const std::vector<EditorTokenSpan> spans = tokenize_grammar_line_with_spans(line, symbols);
	const std::array<int, 2> candidate_columns = {
		std::clamp(cursor_position.column, 0, static_cast<int>(line.size())),
		std::clamp(cursor_position.column - 1, 0, static_cast<int>(line.size()))};

	for (int candidate_column : candidate_columns) {
		for (const EditorTokenSpan &span : spans) {
			if (!is_matchable_identifier_token(span.token.kind)) {
				continue;
			}
			if (candidate_column < span.start_column || candidate_column >= span.end_column) {
				continue;
			}
			if (token_out != nullptr) {
				*token_out = span.token;
			}
			if (range_out != nullptr) {
				range_out->line = cursor_position.line;
				range_out->start_column = span.start_column;
				range_out->end_column = span.end_column;
				range_out->found = true;
			}
			return true;
		}
	}

	return false;
}

EditorIdentifierMatchState build_identifier_match_state(const std::vector<std::string> &lines,
                                                        const GrammarSymbolIndex &symbols,
                                                        const EditorTokenMatch &cursor_position)
{
	EditorIdentifierMatchState state;
	EditorToken active_token;
	if (smart_editor_state.has_selected_identifier &&
	    is_matchable_identifier_token(smart_editor_state.selected_identifier_token.kind)) {
		active_token = smart_editor_state.selected_identifier_token;
		state.active_range.line = smart_editor_state.selected_identifier_line;
		state.active_range.start_column = smart_editor_state.selected_identifier_column;
		state.active_range.end_column =
			smart_editor_state.selected_identifier_column +
			static_cast<int>(smart_editor_state.selected_identifier_token.text.size());
		state.active_range.found = true;
		state.found = true;
		state.text = active_token.text;
	} else if (!find_matchable_identifier_token_near_cursor(lines,
	                                                        symbols,
	                                                        cursor_position,
	                                                        &active_token,
	                                                        &state.active_range)) {
		return state;
	} else {
		state.found = true;
		state.text = active_token.text;
	}

	for (int line_index = 0; line_index < static_cast<int>(lines.size()); ++line_index) {
		const std::vector<EditorTokenSpan> spans =
			tokenize_grammar_line_with_spans(lines[static_cast<std::size_t>(line_index)], symbols);
		for (const EditorTokenSpan &span : spans) {
			if (!is_matchable_identifier_token(span.token.kind) || span.token.text != state.text) {
				continue;
			}
			EditorRange range;
			range.line = line_index;
			range.start_column = span.start_column;
			range.end_column = span.end_column;
			range.found = true;
			state.ranges.push_back(range);
		}
	}

	return state;
}

bool find_delimiter_near_cursor(const std::vector<std::string> &lines,
                                const EditorTokenMatch &cursor_position,
                                char *delimiter_out,
                                EditorRange *range_out)
{
	if (!cursor_position.found || cursor_position.line < 0 ||
	    cursor_position.line >= static_cast<int>(lines.size())) {
		return false;
	}

	const std::string &line = lines[static_cast<std::size_t>(cursor_position.line)];
	const std::size_t comment_start = find_comment_start(line);
	const int code_end = static_cast<int>(comment_start == std::string::npos ? line.size() : comment_start);
	const std::array<int, 2> candidate_columns = {
		std::clamp(cursor_position.column, 0, code_end),
		std::clamp(cursor_position.column - 1, 0, code_end)};

	for (int candidate_column : candidate_columns) {
		if (candidate_column < 0 || candidate_column >= code_end) {
			continue;
		}
		const char c = line[static_cast<std::size_t>(candidate_column)];
		if (!is_opening_delimiter(c) && !is_closing_delimiter(c)) {
			continue;
		}
		if (delimiter_out != nullptr) {
			*delimiter_out = c;
		}
		if (range_out != nullptr) {
			range_out->line = cursor_position.line;
			range_out->start_column = candidate_column;
			range_out->end_column = candidate_column + 1;
			range_out->found = true;
		}
		return true;
	}

	return false;
}

bool find_matching_delimiter_forward(const std::vector<std::string> &lines,
                                     int origin_line,
                                     int origin_column,
                                     char open_delimiter,
                                     char close_delimiter,
                                     EditorRange *range_out)
{
	int depth = 0;
	for (int line_index = origin_line; line_index < static_cast<int>(lines.size()); ++line_index) {
		const std::string &line = lines[static_cast<std::size_t>(line_index)];
		const std::size_t comment_start = find_comment_start(line);
		const int code_end = static_cast<int>(comment_start == std::string::npos ? line.size() : comment_start);
		int start_column = 0;
		if (line_index == origin_line) {
			start_column = origin_column + 1;
		}
		for (int column = start_column; column < code_end; ++column) {
			const char c = line[static_cast<std::size_t>(column)];
			if (c == open_delimiter) {
				++depth;
				continue;
			}
			if (c != close_delimiter) {
				continue;
			}
			if (depth == 0) {
				if (range_out != nullptr) {
					range_out->line = line_index;
					range_out->start_column = column;
					range_out->end_column = column + 1;
					range_out->found = true;
				}
				return true;
			}
			--depth;
		}
	}
	return false;
}

bool find_matching_delimiter_backward(const std::vector<std::string> &lines,
                                      int origin_line,
                                      int origin_column,
                                      char open_delimiter,
                                      char close_delimiter,
                                      EditorRange *range_out)
{
	int depth = 0;
	for (int line_index = origin_line; line_index >= 0; --line_index) {
		const std::string &line = lines[static_cast<std::size_t>(line_index)];
		const std::size_t comment_start = find_comment_start(line);
		const int code_end = static_cast<int>(comment_start == std::string::npos ? line.size() : comment_start);
		int start_column = code_end - 1;
		if (line_index == origin_line) {
			start_column = std::min(origin_column - 1, code_end - 1);
		}
		for (int column = start_column; column >= 0; --column) {
			const char c = line[static_cast<std::size_t>(column)];
			if (c == close_delimiter) {
				++depth;
				continue;
			}
			if (c != open_delimiter) {
				continue;
			}
			if (depth == 0) {
				if (range_out != nullptr) {
					range_out->line = line_index;
					range_out->start_column = column;
					range_out->end_column = column + 1;
					range_out->found = true;
				}
				return true;
			}
			--depth;
		}
	}
	return false;
}

EditorBracketMatchState build_bracket_match_state(const std::vector<std::string> &lines,
                                                  const EditorTokenMatch &cursor_position)
{
	EditorBracketMatchState state;
	if (!find_delimiter_near_cursor(lines, cursor_position, &state.delimiter, &state.active_range)) {
		return state;
	}

	state.found = true;
	if (is_opening_delimiter(state.delimiter)) {
		state.has_pair = find_matching_delimiter_forward(lines,
		                                                 state.active_range.line,
		                                                 state.active_range.start_column,
		                                                 state.delimiter,
		                                                 matching_delimiter(state.delimiter),
		                                                 &state.matching_range);
	} else {
		state.has_pair = find_matching_delimiter_backward(lines,
		                                                  state.active_range.line,
		                                                  state.active_range.start_column,
		                                                  matching_delimiter(state.delimiter),
		                                                  state.delimiter,
		                                                  &state.matching_range);
	}
	return state;
}

void draw_editor_diagnostic_tooltip(const std::vector<std::string> &lines,
                                    const std::vector<EditorDiagnostic> &diagnostics,
                                    ImGuiID editor_id,
                                    const ImRect &editor_rect)
{
	if (diagnostics.empty()) {
		return;
	}

	EditorOverlayLayout layout;
	if (!build_editor_overlay_layout(editor_id, editor_rect, &layout)) {
		return;
	}

	const ImVec2 mouse_position = ImGui::GetIO().MousePos;
	for (const EditorDiagnostic &diagnostic : diagnostics) {
		const ImRect rect = editor_rect_for_range(layout, lines, diagnostic.range);
		if (!rect.Contains(mouse_position)) {
			continue;
		}
		ImGui::BeginTooltip();
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0xca7f7f));
		ImGui::TextUnformatted("Grammar Issue");
		ImGui::PopStyleColor();
		draw_divider();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
		ImGui::TextWrapped("%s", diagnostic.message.c_str());
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
		break;
	}
}

void draw_editor_caret_overlay(const std::vector<std::string> &lines,
                               ImGuiID editor_id,
                               const ImRect &editor_rect,
                               int cursor_offset)
{
	if (editor_id == 0 || lines.empty()) {
		return;
	}

	ImGuiInputTextState *input_state = ImGui::GetInputTextState(editor_id);
	if (input_state == nullptr) {
		return;
	}

	const bool cursor_visible = !ImGui::GetIO().ConfigInputTextCursorBlink ||
	                            input_state->CursorAnim <= 0.0f ||
	                            std::fmod(input_state->CursorAnim, 1.20f) <= 0.80f;
	if (!cursor_visible) {
		return;
	}

	EditorOverlayLayout layout;
	if (!build_editor_overlay_layout(editor_id, editor_rect, &layout)) {
		return;
	}

	const EditorTokenMatch cursor_position = position_for_document_offset(lines, cursor_offset);
	if (!cursor_position.found || cursor_position.line < 0 ||
	    cursor_position.line >= static_cast<int>(lines.size())) {
		return;
	}

	const std::string &line = lines[static_cast<std::size_t>(cursor_position.line)];
	const ImVec2 cursor_screen_pos =
		editor_screen_position_for_column(layout, line, cursor_position.line, cursor_position.column);
	const float caret_width = 1.5f;
	layout.draw_list->PushClipRect(layout.clip_rect.Min, layout.clip_rect.Max, true);
	layout.draw_list->AddLine(cursor_screen_pos,
	                          ImVec2(cursor_screen_pos.x, cursor_screen_pos.y + layout.line_height),
	                          ImGui::ColorConvertFloat4ToU32(color_from_hex(0x3a3632)),
	                          caret_width);
	layout.draw_list->PopClipRect();
}

void draw_smart_editor_highlight_overlay(const std::vector<std::string> &lines,
                                         const GrammarSymbolIndex &symbols,
                                         const std::vector<EditorDiagnostic> &diagnostics,
                                         const EditorIdentifierMatchState &identifier_matches,
                                         const EditorBracketMatchState &bracket_match,
                                         ImGuiID editor_id,
                                         const ImRect &editor_rect)
{
	if (editor_id == 0 || lines.empty()) {
		return;
	}

	EditorOverlayLayout layout;
	if (!build_editor_overlay_layout(editor_id, editor_rect, &layout)) {
		return;
	}

	const int line_count = static_cast<int>(lines.size());
	const int first_visible_line =
		std::clamp(static_cast<int>((layout.clip_rect.Min.y - layout.content_origin.y) / layout.line_height) - 1,
		           0,
		           line_count - 1);
	const int last_visible_line =
		std::clamp(static_cast<int>((layout.clip_rect.Max.y - layout.content_origin.y) / layout.line_height) + 2,
		           0,
		           line_count);

	layout.draw_list->PushClipRect(layout.clip_rect.Min, layout.clip_rect.Max, true);

	for (const EditorRange &range : identifier_matches.ranges) {
		if (!range.found || range.line < first_visible_line || range.line >= last_visible_line) {
			continue;
		}
		ImRect rect;
		if (!editor_rect_for_token_range(layout, lines, symbols, range, &rect)) {
			rect = editor_rect_for_range(layout, lines, range);
		}
		layout.draw_list->AddRectFilled(rect.Min,
		                                rect.Max,
		                                color_for_identifier_match_fill(
		                                	editor_range_equals(range, identifier_matches.active_range)),
		                                3.0f);
	}

	if (bracket_match.found && bracket_match.active_range.line >= first_visible_line &&
	    bracket_match.active_range.line < last_visible_line) {
		ImRect active_rect;
		if (!editor_rect_for_token_range(layout, lines, symbols, bracket_match.active_range, &active_rect)) {
			active_rect = editor_rect_for_range(layout, lines, bracket_match.active_range);
		}
		layout.draw_list->AddRectFilled(active_rect.Min,
		                                active_rect.Max,
		                                color_for_bracket_match_fill(true, bracket_match.has_pair),
		                                3.0f);
		layout.draw_list->AddRect(active_rect.Min,
		                          active_rect.Max,
		                          color_for_diagnostic(bracket_match.has_pair
		                                                   ? EditorDiagnosticSeverity::Warning
		                                                   : EditorDiagnosticSeverity::Error),
		                          3.0f,
		                          0,
		                          1.1f);
	}
	if (bracket_match.has_pair && bracket_match.matching_range.line >= first_visible_line &&
	    bracket_match.matching_range.line < last_visible_line) {
		ImRect matching_rect;
		if (!editor_rect_for_token_range(layout, lines, symbols, bracket_match.matching_range, &matching_rect)) {
			matching_rect = editor_rect_for_range(layout, lines, bracket_match.matching_range);
		}
		layout.draw_list->AddRectFilled(matching_rect.Min,
		                                matching_rect.Max,
		                                color_for_bracket_match_fill(false, true),
		                                3.0f);
		layout.draw_list->AddRect(matching_rect.Min,
		                          matching_rect.Max,
		                          ImGui::ColorConvertFloat4ToU32(color_from_hex(0x8dad97, 0.78f)),
		                          3.0f,
		                          0,
		                          1.0f);
	}

	for (int line_index = first_visible_line; line_index < last_visible_line; ++line_index) {
		const std::vector<EditorToken> tokens =
			tokenize_grammar_line(lines[static_cast<std::size_t>(line_index)], symbols);
		ImVec2 token_pos(layout.content_origin.x,
		                 layout.content_origin.y + line_index * layout.line_height);
		for (const EditorToken &token : tokens) {
			if (token.kind != EditorTokenKind::Plain && !token.text.empty()) {
				layout.draw_list->AddText(ImGui::GetFont(),
				                          layout.font_size,
				                          token_pos,
				                          color_for_editor_token(token),
				                          token.text.c_str());
			}
			token_pos.x += ImGui::CalcTextSize(token.text.c_str(), nullptr, false, -1.0f).x;
		}
	}

	for (const EditorDiagnostic &diagnostic : diagnostics) {
		if (!diagnostic.range.found || diagnostic.range.line < first_visible_line ||
		    diagnostic.range.line >= last_visible_line) {
			continue;
		}
		draw_editor_diagnostic_underline(layout, lines, diagnostic);
	}

	layout.draw_list->PopClipRect();
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

int document_offset_for_position(const std::vector<std::string> &lines, int line_index, int column)
{
	if (lines.empty()) {
		return 0;
	}

	line_index = std::clamp(line_index, 0, static_cast<int>(lines.size()) - 1);
	column = std::clamp(column, 0, static_cast<int>(lines[static_cast<std::size_t>(line_index)].size()));

	int offset = 0;
	for (int index = 0; index < line_index; ++index) {
		offset += static_cast<int>(lines[static_cast<std::size_t>(index)].size());
		offset += 1;
	}
	return offset + column;
}

EditorTokenMatch position_for_document_offset(const std::vector<std::string> &lines, int offset)
{
	if (lines.empty()) {
		return {0, 0, true};
	}

	const int document_length = static_cast<int>(join_lines(lines).size());
	offset = std::clamp(offset, 0, document_length);

	int remaining = offset;
	for (int line_index = 0; line_index < static_cast<int>(lines.size()); ++line_index) {
		const int line_length = static_cast<int>(lines[static_cast<std::size_t>(line_index)].size());
		if (remaining <= line_length) {
			return {line_index, remaining, true};
		}
		remaining -= line_length;
		if (line_index + 1 < static_cast<int>(lines.size())) {
			remaining -= 1;
		}
	}

	const int last_line = static_cast<int>(lines.size()) - 1;
	return {last_line, static_cast<int>(lines.back().size()), true};
}

void jump_to_editor_location(const std::vector<std::string> &lines, int line_index, int column)
{
	const int document_offset = document_offset_for_position(lines, line_index, column);
	smart_editor_state.cursor_column = document_offset;
	smart_editor_state.selection_start = document_offset;
	smart_editor_state.selection_end = document_offset;
	smart_editor_state.requested_cursor_column = document_offset;
	smart_editor_state.request_cursor_sync = true;
	smart_editor_state.requested_selection_start = document_offset;
	smart_editor_state.requested_selection_end = document_offset;
	smart_editor_state.request_selection_sync = true;
	smart_editor_state.request_focus = true;
	smart_editor_state.request_scroll = true;
	smart_editor_state.scroll_target_line = line_index;
}

void select_editor_range(const std::vector<std::string> &lines, int line_index, int start_column, int end_column)
{
	const int start_offset = document_offset_for_position(lines, line_index, start_column);
	const int end_offset = document_offset_for_position(lines, line_index, end_column);
	smart_editor_state.cursor_column = end_offset;
	smart_editor_state.selection_start = start_offset;
	smart_editor_state.selection_end = end_offset;
	smart_editor_state.requested_cursor_column = end_offset;
	smart_editor_state.requested_selection_start = start_offset;
	smart_editor_state.requested_selection_end = end_offset;
	smart_editor_state.request_cursor_sync = true;
	smart_editor_state.request_selection_sync = true;
	smart_editor_state.request_focus = true;
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

std::vector<EditorRange> collect_token_occurrence_ranges(const std::vector<std::string> &lines,
                                                         const GrammarSymbolIndex &symbols,
                                                         const EditorToken &needle)
{
	std::vector<EditorRange> ranges;
	if (needle.text.empty()) {
		return ranges;
	}

	for (int line_index = 0; line_index < static_cast<int>(lines.size()); ++line_index) {
		const std::vector<EditorTokenSpan> spans =
			tokenize_grammar_line_with_spans(lines[static_cast<std::size_t>(line_index)], symbols);
		for (const EditorTokenSpan &span : spans) {
			if (span.token.text != needle.text) {
				continue;
			}
			const bool compatible_matchable_identifier =
				is_matchable_identifier_token(needle.kind) &&
				is_matchable_identifier_token(span.token.kind);
			if (!compatible_matchable_identifier && span.token.kind != needle.kind) {
				continue;
			}
			EditorRange range;
			range.line = line_index;
			range.start_column = span.start_column;
			range.end_column = span.end_column;
			range.found = true;
			ranges.push_back(range);
		}
	}

	return ranges;
}

int find_token_occurrence_index(const std::vector<EditorRange> &ranges,
                                int line,
                                int start_column,
                                int end_column)
{
	for (int index = 0; index < static_cast<int>(ranges.size()); ++index) {
		const EditorRange &range = ranges[static_cast<std::size_t>(index)];
		if (range.line == line &&
		    range.start_column == start_column &&
		    range.end_column == end_column) {
			return index;
		}
	}
	return -1;
}

bool refresh_selected_identifier_from_selection(const std::vector<std::string> &lines,
                                                const GrammarSymbolIndex &symbols)
{
	clear_selected_identifier();

	const int selection_start = std::min(smart_editor_state.selection_start, smart_editor_state.selection_end);
	const int selection_end = std::max(smart_editor_state.selection_start, smart_editor_state.selection_end);
	if (selection_start == selection_end) {
		return false;
	}

	const EditorTokenMatch start = position_for_document_offset(lines, selection_start);
	const EditorTokenMatch end = position_for_document_offset(lines, selection_end);
	if (!start.found || !end.found || start.line != end.line) {
		return false;
	}

	const std::vector<EditorToken> tokens =
		tokenize_grammar_line(lines[static_cast<std::size_t>(start.line)], symbols);
	int token_column = 0;
	for (const EditorToken &token : tokens) {
		const int token_start = token_column;
		const int token_end = token_start + static_cast<int>(token.text.size());
		token_column = token_end;
		if (!is_selectable_identifier_token(token.kind)) {
			continue;
		}
		if (token_start == start.column && token_end == end.column) {
			smart_editor_state.has_selected_identifier = true;
			smart_editor_state.selected_identifier_token = token;
			smart_editor_state.selected_identifier_line = start.line;
			smart_editor_state.selected_identifier_column = start.column;
			return true;
		}
	}

	return false;
}

Rule *find_rule_by_name(const std::string &rule_name)
{
	if (grammar == nullptr) {
		return nullptr;
	}

	for (Rule *rule : grammar->rule_list) {
		if (rule != nullptr && rule->rule_name == rule_name) {
			return rule;
		}
	}
	return nullptr;
}

Variable *find_runtime_variable_by_name(const std::string &variable_name)
{
	if (regeneration_in_progress.load()) {
		return nullptr;
	}

	for (auto it = variable_list.rbegin(); it != variable_list.rend(); ++it) {
		if (*it != nullptr && (*it)->var_name == variable_name) {
			return *it;
		}
	}
	return nullptr;
}

bool draw_selected_identifier_details(const EditorToken &selected)
{
	bool drew_details = false;

	if (Rule *rule = find_rule_by_name(selected.text)) {
		drew_details = true;
		if (!rule->var_name.empty()) {
			ImGui::Text("Repeat Variable: %s", rule->var_name.c_str());
		} else {
			ImGui::Text("Repeat Count: %d", rule->repeat);
		}

		std::string argument_list;
		if (rule->var_counter > 0) {
			std::ostringstream args;
			for (int i = 0; i < rule->var_counter; ++i) {
				if (rule->var_names[i].empty()) {
					continue;
				}
				if (args.tellp() > 0) {
					args << ", ";
				}
				args << rule->var_names[i];
			}
			argument_list = args.str();
		}
		if (!argument_list.empty()) {
			ImGui::TextWrapped("Arguments: %s", argument_list.c_str());
		}

		ImGui::Text("Probability: %.2f", rule->probability);
		ImGui::Text("Sections: %zu / %zu / %zu",
		            rule->section_tokens[0].size(),
		            rule->section_tokens[1].size(),
		            rule->section_tokens[2].size());
		if (rule->alternate != nullptr) {
			ImGui::Text("Alternate Tokens: %zu", rule->alternate->section_tokens[1].size());
		}

		std::string preview = rule->print();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 26.0f);
		ImGui::TextWrapped("%s", preview.c_str());
		ImGui::PopTextWrapPos();
	}

	if (selected.kind == EditorTokenKind::Variable) {
		if (Variable *variable = find_runtime_variable_by_name(selected.text)) {
			drew_details = true;
			ImGui::Text("Runtime Value: %.3f", variable->value);
			ImGui::Text("Range: %.3f .. %.3f", variable->min, variable->max);
			ImGui::TextUnformatted(variable->integer ? "Type: Integer" : "Type: Float");
			if (variable->instance_count > 0) {
				ImGui::Text("Instances: %d", variable->instance_count);
			}
		} else if (regeneration_in_progress.load()) {
			drew_details = true;
			ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
			ImGui::TextUnformatted("Runtime values update after regeneration finishes.");
			ImGui::PopStyleColor();
		}
	}

	return drew_details;
}

void sync_smart_editor_state(const std::string &text)
{
	if (smart_editor_state.synced_text == text) {
		return;
	}
	clear_selected_identifier();
	const int text_length = static_cast<int>(text.size());
	smart_editor_state.cursor_column = std::clamp(smart_editor_state.cursor_column, 0, text_length);
	smart_editor_state.selection_start = std::clamp(smart_editor_state.selection_start, 0, text_length);
	smart_editor_state.selection_end = std::clamp(smart_editor_state.selection_end, 0, text_length);
	smart_editor_state.synced_text = text;
}

void commit_editor_text(AppState *app_state, const std::string &text)
{
	if (app_state == nullptr) {
		return;
	}
	app_state->editor_text = text;
	app_state->editor_dirty = true;
	smart_editor_state.synced_text = app_state->editor_text;
	clear_selected_identifier();
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
                       ImGuiInputTextFlags flags)
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

void set_window_icon(GLFWwindow *window, const std::string &filename)
{
	if (window == nullptr || filename.empty()) {
		return;
	}

	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char *image = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	if (image == nullptr) {
		debugout(std::string("Failed to load window icon: ") + filename + " : " + stbi_failure_reason());
		return;
	}

	GLFWimage icon_image;
	icon_image.width = width;
	icon_image.height = height;
	icon_image.pixels = image;
	glfwSetWindowIcon(window, 1, &icon_image);
	stbi_image_free(image);
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

bool load_ui_image(const std::string &filename, UiImage *image)
{
	if (image == nullptr) {
		return false;
	}
	if (!std::filesystem::exists(filename)) {
		debugout("UI image not found: " + filename);
		return false;
	}

	int width = 0;
	int height = 0;
	int channels = 0;
	if (stbi_info(filename.c_str(), &width, &height, &channels) == 0) {
		errorout(std::string("Failed to inspect image: ") + filename + " : " + stbi_failure_reason());
		return false;
	}

	const GLuint texture = generate_texture(filename);
	if (texture == 0) {
		return false;
	}

	destroy_ui_image(image);
	image->texture = texture;
	image->width = width;
	image->height = height;
	return true;
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
	note_texture_binding_changed("add texture");
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
	request_scene_regeneration(source_text);
	return true;
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

	set_editor_document(app_state, text, normalize_document_path(path), false);
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
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
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
		ImGui::PushStyleColor(ImGuiCol_ChildBg, color_from_hex(0xfffbf7));
		ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xe0d8d1));
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
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
		ImGui::TextWrapped("%s", texture_filenames[i].c_str());
		ImGui::PopStyleColor();
		if (i < texture_list_alpha.size()) {
			ImGui::SliderFloat("Alpha", &texture_list_alpha[i], 0.0f, 1.0f, "%.2f");
		}
		if (ImGui::Button("Up") && i > 0) {
			std::swap(texture_list[i], texture_list[i - 1]);
			std::swap(texture_list_alpha[i], texture_list_alpha[i - 1]);
			std::swap(texture_filenames[i], texture_filenames[i - 1]);
			note_texture_binding_changed("reorder texture up");
			needs_regeneration = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Down") && i + 1 < texture_filenames.size()) {
			std::swap(texture_list[i], texture_list[i + 1]);
			std::swap(texture_list_alpha[i], texture_list_alpha[i + 1]);
			std::swap(texture_filenames[i], texture_filenames[i + 1]);
			note_texture_binding_changed("reorder texture down");
			needs_regeneration = true;
		}
		ImGui::SameLine();
		if (ImGui::Button("Remove")) {
			GLuint texture = static_cast<GLuint>(texture_list[i]);
			glDeleteTextures(1, &texture);
			texture_list.erase(texture_list.begin() + static_cast<long>(i));
			texture_list_alpha.erase(texture_list_alpha.begin() + static_cast<long>(i));
			texture_filenames.erase(texture_filenames.begin() + static_cast<long>(i));
			note_texture_binding_changed("remove texture");
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
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
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
				ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x8f98ab));
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
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
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
	if (regeneration_in_progress.load()) {
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
		ImGui::TextUnformatted(has_pending_regeneration ? "Regenerating (queued update)" : "Regenerating...");
		ImGui::PopStyleColor();
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
	ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xe0d8d1));
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
	GrammarSymbolIndex symbols = build_grammar_symbol_index(lines);

	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
	ImGui::TextUnformatted("Edit the full grammar document. Red underlines mark issues; brackets and identifiers match around the caret.");
	ImGui::PopStyleColor();
	ImGui::Spacing();

	ImGui::BeginChild("SmartEditorRegion", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_NoScrollbar);
	if (app_fonts.mono != nullptr) {
		ImGui::PushFont(app_fonts.mono);
	}

	if (app_state->editor_text.capacity() < 2048) {
		app_state->editor_text.reserve(2048);
	}

	const float footer_height = ImGui::GetTextLineHeightWithSpacing() + 6.0f;
	ImVec2 editor_size = ImGui::GetContentRegionAvail();
	editor_size.y = std::max(120.0f, editor_size.y - footer_height);

	ImGui::PushStyleColor(ImGuiCol_FrameBg, color_from_hex(0xfffdfa));
	ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xd6ccd9, 0.85f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 2.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);

	if (smart_editor_state.request_focus) {
		ImGui::SetKeyboardFocusHere();
		smart_editor_state.request_focus = false;
	}

	EditorLineInputCallbackUserData callback_data{
		&app_state->editor_text,
		&smart_editor_state.cursor_column,
		&smart_editor_state.selection_start,
		&smart_editor_state.selection_end};
	const ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_CallbackResize |
	                                        ImGuiInputTextFlags_CallbackAlways |
	                                        ImGuiInputTextFlags_AllowTabInput;
	const bool text_changed = ImGui::InputTextMultiline("##smart_editor_document",
	                                                    app_state->editor_text.data(),
	                                                    app_state->editor_text.capacity() + 1,
	                                                    editor_size,
	                                                    input_flags,
	                                                    editor_line_input_callback,
	                                                    &callback_data);
	const ImGuiID editor_id = ImGui::GetItemID();
	const ImRect editor_rect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	const bool editor_hovered = ImGui::IsItemHovered();

	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(3);

	if (smart_editor_state.request_scroll) {
		if (ImGuiInputTextState *input_state = ImGui::GetInputTextState(editor_id)) {
			input_state->CursorFollow = true;
		}
		if (smart_editor_state.scroll_target_line >= 0) {
			EditorOverlayLayout layout;
			if (build_editor_overlay_layout(editor_id, editor_rect, &layout)) {
				const float visible_height = layout.window->InnerRect.GetHeight();
				const float target_scroll_y = std::max(
					0.0f,
					smart_editor_state.scroll_target_line * layout.line_height - visible_height * 0.35f);
				ImGui::SetScrollY(layout.window, target_scroll_y);
			}
		}
		smart_editor_state.request_scroll = false;
		smart_editor_state.scroll_target_line = -1;
	}

	if (text_changed) {
		commit_editor_text(app_state, app_state->editor_text);
		lines = current_editor_lines(app_state->editor_text);
		symbols = build_grammar_symbol_index(lines);
	}
	const bool has_identifier_selection = refresh_selected_identifier_from_selection(lines, symbols);
	const EditorTokenMatch cursor_position = position_for_document_offset(lines, smart_editor_state.cursor_column);
	const std::vector<EditorDiagnostic> diagnostics = build_editor_diagnostics(lines);
	const EditorIdentifierMatchState identifier_matches =
		build_identifier_match_state(lines, symbols, cursor_position);
	const EditorBracketMatchState bracket_match = build_bracket_match_state(lines, cursor_position);
	draw_smart_editor_highlight_overlay(lines,
	                                    symbols,
	                                    diagnostics,
	                                    identifier_matches,
	                                    bracket_match,
	                                    editor_id,
	                                    editor_rect);
	draw_editor_caret_overlay(lines, editor_id, editor_rect, smart_editor_state.cursor_column);
	if (editor_hovered && !ImGui::IsPopupOpen("IdentifierSelectionTooltip")) {
		draw_editor_diagnostic_tooltip(lines, diagnostics, editor_id, editor_rect);
	}
	if (has_identifier_selection && editor_hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		ImGui::OpenPopup("IdentifierSelectionTooltip");
	}

	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
	ImGui::Text("Line %d, Column %d  |  %zu lines",
	            cursor_position.line + 1,
	            cursor_position.column + 1,
	            lines.size());
	if (!diagnostics.empty()) {
		ImGui::SameLine();
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0xca7f7f));
		ImGui::Text("| %zu issue%s", diagnostics.size(), diagnostics.size() == 1 ? "" : "s");
		ImGui::PopStyleColor();
	}
	if (identifier_matches.found) {
		ImGui::SameLine();
		ImGui::Text("| Matches: %zu", identifier_matches.ranges.size());
	}
	if (bracket_match.found) {
		ImGui::SameLine();
		if (bracket_match.has_pair) {
			ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x7ea88e));
			ImGui::TextUnformatted("| Bracket matched");
		} else {
			ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0xca7f7f));
			ImGui::Text("| Unmatched %c", bracket_match.delimiter);
		}
		ImGui::PopStyleColor();
	}
	if (smart_editor_state.has_selected_identifier) {
		ImGui::SameLine();
		ImGui::Text("| Selected: %s", smart_editor_state.selected_identifier_token.text.c_str());
	}
	ImGui::PopStyleColor();

	if (ImGui::BeginPopup("IdentifierSelectionTooltip")) {
		if (smart_editor_state.has_selected_identifier) {
			const EditorToken &selected = smart_editor_state.selected_identifier_token;
			const std::vector<EditorRange> occurrence_ranges =
				collect_token_occurrence_ranges(lines, symbols, selected);
			ImGui::TextUnformatted(selected.text.c_str());
			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
			ImGui::TextUnformatted(label_for_editor_token_kind(selected.kind).c_str());
			ImGui::PopStyleColor();
			draw_divider();
			ImGui::Text("Line %d, Column %d",
			            smart_editor_state.selected_identifier_line + 1,
			            smart_editor_state.selected_identifier_column + 1);
			ImGui::Text("Occurrences: %zu", occurrence_ranges.size());
			if (draw_selected_identifier_details(selected)) {
				draw_divider();
			}
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
			ImGui::TextWrapped("%s", selected.tooltip.c_str());
			ImGui::PopTextWrapPos();
			const int selected_end_column =
				smart_editor_state.selected_identifier_column +
				static_cast<int>(selected.text.size());
			const int occurrence_index =
				find_token_occurrence_index(occurrence_ranges,
				                            smart_editor_state.selected_identifier_line,
				                            smart_editor_state.selected_identifier_column,
				                            selected_end_column);
			const bool has_previous_occurrence = occurrence_index > 0;
			const bool has_next_occurrence =
				occurrence_index >= 0 &&
				occurrence_index + 1 < static_cast<int>(occurrence_ranges.size());
			if (!occurrence_ranges.empty()) {
				draw_divider();
				if (occurrence_index >= 0) {
					ImGui::Text("Instance %d of %zu",
					            occurrence_index + 1,
					            occurrence_ranges.size());
				} else {
					ImGui::Text("Instances: %zu", occurrence_ranges.size());
				}
				if (has_previous_occurrence || has_next_occurrence) {
					if (has_previous_occurrence) {
						if (ImGui::ArrowButton("##prev_identifier_occurrence", ImGuiDir_Left)) {
							const EditorRange &range =
								occurrence_ranges[static_cast<std::size_t>(occurrence_index - 1)];
							select_editor_range(lines,
							                    range.line,
							                    range.start_column,
							                    range.end_column);
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
						if (ImGui::ArrowButton("##next_identifier_occurrence", ImGuiDir_Right)) {
							const EditorRange &range =
								occurrence_ranges[static_cast<std::size_t>(occurrence_index + 1)];
							select_editor_range(lines,
							                    range.line,
							                    range.start_column,
							                    range.end_column);
						}
						ImGui::SameLine();
						ImGui::TextUnformatted("Next");
					}
				}
			}
		} else {
			ImGui::CloseCurrentPopup();
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
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
	ImGui::TextUnformatted("Edit grammars, manage file outputs, and curate texture bindings.");
	ImGui::PopStyleColor();
	const std::string document_label = display_document_name(app_state);
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x6f6862));
	ImGui::TextWrapped("%s", document_label.c_str());
	ImGui::PopStyleColor();
	draw_divider();

	if (ImGui::Button("New")) {
		create_new_document(app_state);
	}
	ImGui::SameLine();
	if (ImGui::Button("Open...")) {
		open_file_dialog(app_state, FileDialogAction::Open);
	}
	ImGui::SameLine();
	if (ImGui::Button("Save")) {
		if (grammar_filename.empty()) {
			open_file_dialog(app_state, FileDialogAction::SaveAs);
		} else {
			save_document_to_path(app_state, grammar_filename);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Save As...")) {
		open_file_dialog(app_state, FileDialogAction::SaveAs);
	}
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, color_from_hex(0xd7c4e8));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color_from_hex(0xcbb8e2));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, color_from_hex(0xbba7d7));
	if (ImGui::Button("Run")) {
		regenerate_scene(app_state->editor_text);
	}
	ImGui::PopStyleColor(3);
	ImGui::SameLine();
	if (ImGui::Button("Export PLY")) {
		std::filesystem::path export_path = grammar_filename.empty()
		                                        ? std::filesystem::path("untitled.ply")
		                                        : std::filesystem::path(grammar_filename).replace_extension(".ply");
		export_ply(export_path.string());
	}

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

	draw_file_dialog(app_state);
}

void draw_main_layout(AppState *app_state, float delta_seconds)
{
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float header_height = 82.0f;
	const float splitter_thickness = 10.0f;

	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
	                                ImGuiWindowFlags_NoMove |
	                                ImGuiWindowFlags_NoSavedSettings;
	ImGui::Begin("Progen3d", nullptr, window_flags);

	begin_surface("HeaderSurface", ImVec2(0.0f, header_height), true, ImVec2(8.0f, 6.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 2.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, 2.0f));
	ImGui::BeginGroup();
	if (header_logo.texture != 0) {
		const float logo_height = 40.0f;
		const float aspect_ratio =
			header_logo.height > 0 ? static_cast<float>(header_logo.width) / header_logo.height : 1.0f;
		ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(header_logo.texture)),
		             ImVec2(logo_height * aspect_ratio, logo_height),
		             ImVec2(0.0f, 1.0f),
		             ImVec2(1.0f, 0.0f));
		ImGui::SameLine(0.0f, 8.0f);
	}
	ImGui::BeginGroup();
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
	ImGui::TextUnformatted("PROCEDURAL MODEL STUDIO");
	ImGui::PopStyleColor();
	if (app_fonts.title != nullptr) {
		ImGui::PushFont(app_fonts.title);
	}
	ImGui::TextUnformatted("Progen3d");
	if (app_fonts.title != nullptr) {
		ImGui::PopFont();
	}
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x6f6862));
	ImGui::TextWrapped("Grammar authoring, material curation, and live 3D inspection.");
	ImGui::PopStyleColor();
	ImGui::EndGroup();
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
		draw_stat_block("Active File", display_document_name(app_state), true, true);
		ImGui::TableNextColumn();
		draw_stat_block("Rules", std::to_string(grammar != nullptr ? grammar->rule_list.size() : 0U));
		ImGui::TableNextColumn();
		draw_stat_block("Tokens", std::to_string(grammar != nullptr ? grammar->tokens_new.size() : 0U));
		ImGui::TableNextColumn();
		draw_stat_block("Textures", std::to_string(texture_list.size()));
		ImGui::EndTable();
	}
	ImGui::PopStyleVar(2);
	end_surface();

	ImGui::Dummy(ImVec2(0.0f, 0.0f));
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
	set_window_icon(window, "snap/gui/progen3d.png");

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
	app_state.file_dialog.directory = default_dialog_directory();
	load_ui_image("pg3d-cubes.png", &header_logo);
	load_default_textures();

	if (!load_grammar_into_editor(&app_state, grammar_filename)) {
		set_editor_document(&app_state, default_new_document_text(), "", false);
	}
	regenerate_scene(app_state.editor_text);

	double last_time = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		process_completed_regeneration();

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
		glClearColor(0.972f, 0.957f, 0.933f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glfwSwapBuffers(window);
	}

	if (regeneration_worker.joinable()) {
		regeneration_worker.join();
	}
	{
		std::unique_ptr<RegenerationResult> result;
		{
			std::lock_guard<std::mutex> lock(completed_regeneration_mutex);
			result = std::move(completed_regeneration_result);
		}
		if (result != nullptr) {
			if (result->grammar != nullptr) {
				delete result->grammar;
			}
			if (!result->temp_filename.empty()) {
				std::error_code error;
				std::filesystem::remove(result->temp_filename, error);
			}
		}
	}
	destroy_grammar();
	clear_variable_pool();
	destroy_ui_image(&header_logo);
	destroy_textures();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	shutdown_renderer();
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
