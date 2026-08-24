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
#include <functional>
#include <iomanip>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ProGen3dGl.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/ext.hpp>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "AppPaths.h"
#include "BackendApiClient.h"
#include "CloudAuthWorkflow.h"
#include "Context.h"
#include "building/model/SmallModernBuildingModel.h"
#include "editor/application/ApplicationStartupProgress.h"
#include "editor/application/EditorRuntimeEnvironment.h"
#include "editor/application/Progen3dEditorApplication.h"
#include "editor/controller/PreviewCameraMotionController.h"
#include "editor/controller/PreviewTimelineController.h"
#include "editor/controller/SceneSelectionController.h"
#include "editor/model/ApplicationLog.h"
#include "editor/model/EditorWorkspaceSession.h"
#include "editor/model/GrammarEditorInteractionState.h"
#include "editor/model/PreviewTexturePresentation.h"
#include "editor/model/ScenePrimitiveInspection.h"
#include "editor/model/SpatialObjectInspection.h"
#include "editor/model/GrammarSymbolIndex.h"
#include "editor/model/ScenePreviewSession.h"
#include "editor/relationship/SpatialObjectPrimitiveAssociationIndex.h"
#include "editor/presentation/AiAssistantPanel.h"
#include "editor/presentation/AiGrammarProposalPanel.h"
#include "editor/presentation/ApplicationHeaderPanel.h"
#include "editor/presentation/AuthenticationPanel.h"
#include "editor/presentation/CloudDocumentDialog.h"
#include "editor/presentation/GrammarAutocompletePresentation.h"
#include "editor/presentation/ConsolePanel.h"
#include "editor/presentation/DiagnosticsPanel.h"
#include "editor/presentation/ElectricalControlsPanel.h"
#include "editor/presentation/EditorWorkspaceWindow.h"
#include "editor/presentation/GrammarDiagnosticPresentation.h"
#include "editor/presentation/GrammarEditorPanel.h"
#include "editor/presentation/GrammarSymbolInspectorPanel.h"
#include "editor/presentation/GrammarSyntaxPresentation.h"
#include "editor/presentation/LightingPanel.h"
#include "editor/presentation/MaterialLibraryPanel.h"
#include "editor/presentation/PreviewOrientationControl.h"
#include "editor/presentation/PreviewTimelinePanel.h"
#include "editor/presentation/RenderSettingsPanel.h"
#include "editor/presentation/ScenePreviewPanel.h"
#include "editor/presentation/SceneInspectorPanel.h"
#include "editor/presentation/SpatialObjectInspectorPanel.h"
#include "editor/service/BackendAiGrammarProposalService.h"
#include "editor/service/BackendTextureLibraryRepository.h"
#include "editor/service/DocumentPersistenceService.h"
#include "editor/service/FirebaseAuthenticationService.h"
#include "editor/service/GrammarCompilationService.h"
#include "editor/service/MeshExportService.h"
#include "editor/service/LocalFileDialogService.h"
#include "editor/service/NativeLocalFileDialogService.h"
#include "editor/service/PreviewFramebufferCaptureService.h"
#include "editor/service/SceneRegenerationCoordinator.h"
#include "editor/service/SpatialDeclarationCompletionService.h"
#include "editor/service/SpatialObjectInspectionService.h"
#include "editor/service/SpatialOverlayEvidenceService.h"
#include "editor/service/StructuredShapeCompletionService.h"
#include "FirebaseAuth.h"
#include "Solution.h"
#include "StlCatalog.h"
#include "geometry/model/AxialProfileShapeSpecification.h"
#include "lighting/model/LightGizmoProjectionContext.h"
#include "lighting/model/LightingSceneDefinition.h"
#include "lighting/service/LightGizmoOverlayService.h"
#include "lighting/service/LightGizmoPickingService.h"
#include "lighting/service/LightingScenePersistenceService.h"
#include "electrical/service/LightingStateEvaluator.h"
#include "geometry/model/BotanicalBladeShapeSpecification.h"
#include "geometry/model/TaperedSweepShapeSpecification.h"
#include "vegetation/model/PlantShapeSpecification.h"
#include "vegetation/model/ScatterRegionShapeSpecification.h"
#include "vegetation/model/VineShapeSpecification.h"
#include "geometry/service/ProceduralShapeCatalogRepository.h"
#include "grammar.h"
#include "GrammarRuntime.h"
#include "geometry/model/ShapeSpecification.h"
#include "imgui_render.h"
#include "spatial/model/SpatialBuildingModel.h"
#include "spatial/model/SpatialObjectId.h"
#include "spatial/service/SpatialObjectBoundaryDerivationService.h"
#include "stbimage/stb_image.h"

void errorout(std::string error_str);
void debugout(const std::string &message);
void debugstate(const std::string &key, const std::string &message);
void upload_fulltext(std::string fulltext);

EditorWorkspaceSession *running_workspace_session = nullptr;
LocalFileDialogService *running_local_file_dialog_service = nullptr;
GLFWwindow *running_application_window = nullptr;
bool application_exit_authorized = false;

EditorWorkspaceSession &current_workspace_session()
{
	if (running_workspace_session == nullptr) {
		throw std::logic_error("No editor workspace is bound to the running application");
	}
	return *running_workspace_session;
}

ScenePreviewSession &current_preview_session()
{
	return current_workspace_session().preview;
}

std::optional<PreviewOrientation> camera_orientation_for_preview_view(
	PreviewViewSelection view_selection)
{
	switch (view_selection) {
	case PreviewViewSelection::Front:
		return PreviewOrientation::PositiveZ;
	case PreviewViewSelection::Back:
		return PreviewOrientation::NegativeZ;
	case PreviewViewSelection::Left:
		return PreviewOrientation::NegativeX;
	case PreviewViewSelection::Right:
		return PreviewOrientation::PositiveX;
	case PreviewViewSelection::Top:
		return PreviewOrientation::PositiveY;
	case PreviewViewSelection::Bottom:
		return PreviewOrientation::NegativeY;
	case PreviewViewSelection::Isometric:
		return PreviewOrientation::Isometric;
	case PreviewViewSelection::Unspecified:
		return std::nullopt;
	}
	return std::nullopt;
}

const char *preview_view_selection_name(PreviewViewSelection view_selection)
{
	switch (view_selection) {
	case PreviewViewSelection::Front:
		return "front";
	case PreviewViewSelection::Back:
		return "back";
	case PreviewViewSelection::Left:
		return "left";
	case PreviewViewSelection::Right:
		return "right";
	case PreviewViewSelection::Top:
		return "top";
	case PreviewViewSelection::Bottom:
		return "bottom";
	case PreviewViewSelection::Isometric:
		return "isometric";
	case PreviewViewSelection::Unspecified:
		return "unspecified";
	}
	return "unspecified";
}

struct PreviewObjectExclusionEvidence {
	std::size_t requested_object_count = 0;
	std::size_t matched_object_count = 0;
	std::size_t hidden_primitive_count = 0;
};

PreviewObjectExclusionEvidence exclude_spatial_objects_from_preview(
	const std::vector<std::string> &object_ids)
{
	PreviewObjectExclusionEvidence evidence;
	evidence.requested_object_count = object_ids.size();
	GeneratedSceneSnapshot *snapshot =
		current_preview_session().last_valid_scene_snapshot.get();
	Context *context = snapshot != nullptr ? snapshot->generationContext() : nullptr;
	const SpatialBuildingModel *model =
		snapshot != nullptr ? snapshot->spatialBuildingModel() : nullptr;
	if (context == nullptr || model == nullptr) {
		return evidence;
	}

	const SpatialObjectPrimitiveAssociationIndex association_index =
		SpatialObjectPrimitiveAssociationIndex::fromModel(*model);
	std::unordered_set<std::size_t> hidden_primitive_indices;
	for (const std::string &object_id_value : object_ids) {
		const std::vector<std::size_t> &primitive_indices =
			association_index.primitivesForObject(SpatialObjectId(object_id_value));
		if (!primitive_indices.empty()) {
			++evidence.matched_object_count;
		}
		for (const std::size_t primitive_index : primitive_indices) {
			if (primitive_index >= context->primitive_instances.size() ||
			    !hidden_primitive_indices.insert(primitive_index).second) {
				continue;
			}
			context->primitive_instances[primitive_index].removed = true;
			++evidence.hidden_primitive_count;
		}
	}
	return evidence;
}

std::unique_ptr<GeneratedSceneSnapshot> &current_scene_snapshot()
{
	return current_preview_session().last_valid_scene_snapshot;
}

bool &authentication_panel_is_visible()
{
	return current_workspace_session().layout.authentication_panel_visible;
}

bool &preview_playback_active()
{
	return current_preview_session().timeline.playing;
}

float &preview_scale_factor()
{
	return current_preview_session().camera.scaleFactor();
}

float &preview_azimuth_degrees()
{
	return current_preview_session().camera.azimuthDegrees();
}

float &preview_elevation_degrees()
{
	return current_preview_session().camera.elevationDegrees();
}

float &preview_roll_degrees()
{
	return current_preview_session().camera.rollDegrees();
}

float &preview_target_x_coordinate()
{
	return current_preview_session().camera.targetX();
}

float &preview_target_y_coordinate()
{
	return current_preview_session().camera.targetY();
}

float &preview_target_z_coordinate()
{
	return current_preview_session().camera.targetZ();
}

float &preview_camera_distance_value()
{
	return current_preview_session().camera.distance();
}

float &preview_time_scale()
{
	return current_preview_session().timeline.time_scale;
}

float &preview_simulation_accumulated_time()
{
	return current_preview_session().timeline.simulation_accumulator;
}

float &preview_simulation_interpolation_alpha()
{
	return current_preview_session().timeline.simulation_alpha;
}

double &preview_grammar_target_time()
{
	return current_preview_session().timeline.grammar_target_time;
}

double &preview_grammar_request_accumulator()
{
	return current_preview_session().timeline.grammar_request_accumulator;
}

bool &connection_overlay_visible()
{
	return current_preview_session().overlays.connection_overlay_visible;
}

bool &connection_axes_visible()
{
	return current_preview_session().overlays.connection_axes_visible;
}

bool &connection_surfaces_visible()
{
	return current_preview_session().overlays.connection_surfaces_visible;
}

bool &connection_features_visible()
{
	return current_preview_session().overlays.connection_features_visible;
}

bool &connection_bounds_visible()
{
	return current_preview_session().overlays.connection_bounds_visible;
}

bool &connection_legend_visible()
{
	return current_preview_session().overlays.connection_legend_visible;
}

bool &axial_profile_overlay_visible()
{
	return current_preview_session().overlays.axial_profile_overlay_visible;
}

bool &spatial_overlay_visible()
{
	return current_preview_session().overlays.spatial_overlay_visible;
}

bool &spatial_object_frames_visible()
{
	return current_preview_session().overlays.spatial_object_frames_visible;
}

bool &spatial_interfaces_visible()
{
	return current_preview_session().overlays.spatial_interfaces_visible;
}

bool &spatial_connections_visible()
{
	return current_preview_session().overlays.spatial_connections_visible;
}

bool &spatial_constraints_visible()
{
	return current_preview_session().overlays.spatial_constraints_visible;
}

bool &spatial_contacts_visible()
{
	return current_preview_session().overlays.spatial_contacts_visible;
}

bool &spatial_clearances_visible()
{
	return current_preview_session().overlays.spatial_clearances_visible;
}

bool &spatial_bounds_visible()
{
	return current_preview_session().overlays.spatial_bounds_visible;
}

bool &building_function_allocations_visible()
{
	return current_preview_session().overlays.building_function_allocations_visible;
}

bool &building_service_flows_visible()
{
	return current_preview_session().overlays.building_service_flows_visible;
}

bool &building_requirement_status_visible()
{
	return current_preview_session().overlays.building_requirement_status_visible;
}

bool &building_pending_evidence_visible()
{
	return current_preview_session().overlays.building_pending_evidence_visible;
}

Grammar *grammar = nullptr;
std::mt19937_64 grammar_rng;
std::mt19937_64 effects_rng;
uint64_t grammar_session_seed = 0;


std::vector<PreviewMaterial> active_materials;
std::unordered_map<std::string, PreviewMaterial> material_cache;

PreviewTimelineController preview_timeline_controller;
SceneSelectionController scene_selection_controller;
constexpr float kMinSimulationTimeScale = 0.01f;
constexpr float kMaxSimulationTimeScale = 8.0f;
constexpr float kPreviewSimulationStep = 1.0f / 120.0f;
constexpr int kMaxPreviewSimulationStepsPerFrame = 8;
constexpr double kGrammarAnimationSampleStep = 1.0 / 60.0;
uint64_t next_grammar_design_nonce = 0;
uint64_t active_grammar_design_nonce = 0;
constexpr float kMinEditorFontScale = 0.75f;
constexpr float kMaxEditorFontScale = 2.25f;
PreviewTextureDebugView preview_texture_debug_view = PreviewTextureDebugView::Shaded;
int preview_mapping_mode_override = -1;

std::string trim_copy(const std::string &input)
{
	const std::size_t start = input.find_first_not_of(" \t\r\n");
	if (start == std::string::npos) {
		return "";
	}
	const std::size_t end = input.find_last_not_of(" \t\r\n");
	return input.substr(start, end - start + 1);
}

namespace {

struct ValidationTokenSpan {
	std::string text;
	int line = -1;
	int start_column = 0;
	int end_column = 0;
};

struct ValidationRuleBlock {
	int start_line = -1;
	int end_line = -1;
	std::vector<ValidationTokenSpan> tokens;
	std::string content;
};

struct AppFonts {
	ImFont *body = nullptr;
	ImFont *small = nullptr;
	ImFont *section = nullptr;
	ImFont *title = nullptr;
	ImFont *mono = nullptr;
};

struct UiImage {
	GLuint texture = 0;
	int width = 0;
	int height = 0;
};

struct PreviewInteractionCameraState {
	glm::vec3 target{0.0f};
	glm::vec3 scaled_target{0.0f};
	glm::vec3 position{0.0f};
	glm::mat4 view{1.0f};
	glm::mat4 projection{1.0f};
	glm::vec3 view_direction{1.0f, 0.0f, 0.0f};
};

struct ViewCubeFace {
	const char *label = "";
	glm::vec3 normal{0.0f};
	std::array<glm::vec3, 4> corners{};
	unsigned int fill_color = 0xffffff;
	float depth = 0.0f;
	bool visible = false;
};

std::size_t find_comment_start(const std::string &line);
void add_app_state_grammar_diagnostic(EditorWorkspaceSession *app_state, const GrammarDiagnostic &diagnostic);
int line_code_end_column(const std::string &line);
std::vector<std::string> current_editor_lines(const std::string &text);
GrammarDiagnostic make_line_grammar_diagnostic(const std::vector<std::string> &lines,
                                               int line_number,
                                               const std::string &message);
GrammarDiagnostic make_token_grammar_diagnostic(const ValidationTokenSpan &token,
                                                const std::string &message);
void clear_texture_library_state_internal(EditorWorkspaceSession *app_state);
void clear_ai_assistant_state_internal(EditorWorkspaceSession *app_state);
void invalidate_texture_materials_internal(EditorWorkspaceSession *app_state);
void refresh_texture_library_internal(EditorWorkspaceSession *app_state, bool preserve_status_message = false);
void refresh_ai_threads_internal(EditorWorkspaceSession *app_state, bool preserve_status_message = false);
void open_cloud_dialog(EditorWorkspaceSession *app_state, CloudDialogAction action);
std::vector<ValidationTokenSpan> tokenize_validation_rule_block(const std::vector<std::string> &lines,
                                                                int start_line,
                                                                int end_line);
std::string join_validation_tokens(const std::vector<ValidationTokenSpan> &tokens,
                                   std::size_t start_index,
                                   std::size_t end_index);
std::vector<ValidationRuleBlock> build_validation_rule_blocks(const std::vector<std::string> &lines);
PreviewInteractionCameraState build_preview_camera_state(const ImVec2 &viewport_size);
int pick_preview_instance_index(const PreviewInteractionCameraState &camera,
                                const ImRect &viewport_rect,
                                Context *context);
void select_preview_instance_in_editor(EditorWorkspaceSession *app_state, Context *context, int instance_index);
void fit_preview_camera_to_scene(EditorWorkspaceSession *app_state, const ImVec2 &viewport_size);
void sanitize_preview_selection(EditorWorkspaceSession *app_state);
std::vector<PreviewOutlineBatch> build_preview_outline_batches(Context *context,
                                                               const EditorWorkspaceSession *app_state);
bool draw_preview_view_cube(const ImRect &preview_rect, bool *capture_pointer);
void draw_material_metric_bar(const char *label, float value, unsigned int fill_rgb);
void destroy_preview_material_textures(PreviewMaterial *material);
void auto_generate_missing_texture_maps(PreviewMaterial *material);
void draw_render_settings_controls(EditorWorkspaceSession *app_state);
std::string backend_credit_plan_label(const BackendCreditSummary &credits);
std::string backend_credit_available_label(const BackendCreditSummary &credits);
std::string backend_credit_reserved_label(const BackendCreditSummary &credits);
std::string ai_mode_label(const std::string &mode);
std::string ai_mode_help_text(const std::string &mode);
bool ai_mode_prefers_prompt(const std::string &mode);
std::string build_default_new_document_text_internal();
bool validate_grammar(Grammar *gram, EditorWorkspaceSession *app_state, bool quiet = false);
GrammarSyntaxPresentation &grammar_syntax_presentation();

uint64_t mix_seed64(uint64_t value)
{
	value ^= value >> 30;
	value *= 0xbf58476d1ce4e5b9ULL;
	value ^= value >> 27;
	value *= 0x94d049bb133111ebULL;
	value ^= value >> 31;
	return value;
}

uint64_t hash_grammar_source(const std::string &source_text)
{
	uint64_t hash = 1469598103934665603ULL;
	for(unsigned char character : source_text){
		hash ^= static_cast<uint64_t>(character);
		hash *= 1099511628211ULL;
	}
	return hash;
}

void seed_engine(std::mt19937_64 *engine, uint64_t seed, uint64_t stream_tag)
{
	if(engine == nullptr)return;
	const uint64_t mixed_seed = mix_seed64(seed ^ stream_tag);
	std::seed_seq seed_sequence{
		static_cast<uint32_t>(mixed_seed & 0xffffffffu),
		static_cast<uint32_t>(mixed_seed >> 32),
		static_cast<uint32_t>(stream_tag & 0xffffffffu),
		static_cast<uint32_t>(stream_tag >> 32)};
	engine->seed(seed_sequence);
}

void seed_grammar_rng_for_generation(const std::string &source_text,
                                     uint64_t design_nonce)
{
	// A design nonce identifies one stochastic design. Time-sampled rebuilds
	// reuse that nonce so R/R* values and stochastic production choices remain
	// stable while only the built-in `t` input changes.
	seed_engine(&grammar_rng,
	            grammar_session_seed ^ hash_grammar_source(source_text) ^
	                mix_seed64(design_nonce),
	            0x4752414d4d415230ULL); // "GRAMMAR0"
}

struct EditorOverlayLayout {
	ImGuiWindow *window = nullptr;
	ImDrawList *draw_list = nullptr;
	ImRect clip_rect;
	ImVec2 content_origin;
	float line_height = 0.0f;
	float font_size = 0.0f;
};

ApplicationLog application_log;
std::unordered_map<std::string, std::string> debug_state_messages;
AppFonts app_fonts;
UiImage header_logo;
GrammarEditorInteractionState smart_editor_state;
GrammarEditorPanel grammar_editor_panel;
GrammarSymbolInspectorPanel grammar_symbol_inspector_panel;
GrammarAutocompletePresentation grammar_autocomplete_presentation;
AiAssistantPanel ai_assistant_panel;
AiGrammarProposalPanel ai_grammar_proposal_panel;
ApplicationHeaderPanel application_header_panel;
AuthenticationPanel authentication_panel;
CloudDocumentDialog cloud_document_dialog;
EditorWorkspaceWindow editor_workspace_window;
MaterialLibraryPanel material_library_panel;
PreviewOrientationControl preview_orientation_control;
PreviewCameraMotionController preview_camera_motion_controller;
PreviewTimelinePanel preview_timeline_panel;
RenderSettingsPanel render_settings_panel;
ScenePreviewPanel scene_preview_panel;
ConsolePanel console_panel;
DiagnosticsPanel diagnostics_panel;
ElectricalControlsPanel electrical_controls_panel;
LightingPanel lighting_panel;
SceneInspectorPanel scene_inspector_panel;
SpatialObjectInspectorPanel spatial_object_inspector_panel;
SpatialObjectInspectionService spatial_object_inspection_service;
SpatialOverlayEvidenceService spatial_overlay_evidence_service;
std::unique_ptr<GrammarCompilationService> grammar_compilation_service;
std::unique_ptr<SceneRegenerationCoordinator> scene_regeneration_coordinator;
using StartupProgressCallback = std::function<bool(float, const std::string &)>;

bool scene_generation_in_progress()
{
	return scene_regeneration_coordinator != nullptr &&
	       scene_regeneration_coordinator->generationInProgress();
}

bool scene_generation_queued()
{
	return scene_regeneration_coordinator != nullptr &&
	       scene_regeneration_coordinator->hasPendingRequest();
}

bool scene_generation_scheduled()
{
	return scene_regeneration_coordinator != nullptr &&
	       scene_regeneration_coordinator->hasScheduledRequest();
}

double scene_generation_scheduled_start_time()
{
	return scene_regeneration_coordinator != nullptr
		       ? scene_regeneration_coordinator->scheduledStartTime()
		       : 0.0;
}

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
	style.FramePadding = ImVec2(8.0f, 4.5f);
	style.CellPadding = ImVec2(6.0f, 4.5f);
	style.ItemSpacing = ImVec2(8.0f, 5.5f);
	style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
	style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
	style.IndentSpacing = 12.0f;
	style.ScrollbarSize = 10.0f;
	style.GrabMinSize = 8.0f;
	style.WindowBorderSize = 1.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameBorderSize = 1.0f;
	style.TabBorderSize = 0.0f;
	style.WindowRounding = 12.0f;
	style.ChildRounding = 10.0f;
	style.FrameRounding = 7.0f;
	style.PopupRounding = 9.0f;
	style.ScrollbarRounding = 12.0f;
	style.GrabRounding = 7.0f;
	style.TabRounding = 8.0f;

	ImVec4 *colors = style.Colors;
	colors[ImGuiCol_Text] = color_from_hex(0x1f2933);
	colors[ImGuiCol_TextDisabled] = color_from_hex(0x70808f);
	colors[ImGuiCol_WindowBg] = color_from_hex(0xf2f5f9);
	colors[ImGuiCol_ChildBg] = color_from_hex(0xfcfdff);
	colors[ImGuiCol_PopupBg] = color_from_hex(0xffffff, 0.98f);
	colors[ImGuiCol_Border] = color_from_hex(0xd4dce6, 0.96f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	colors[ImGuiCol_FrameBg] = color_from_hex(0xf4f7fb);
	colors[ImGuiCol_FrameBgHovered] = color_from_hex(0xebf1f8);
	colors[ImGuiCol_FrameBgActive] = color_from_hex(0xe2ebf5);
	colors[ImGuiCol_TitleBg] = color_from_hex(0xf7f9fc);
	colors[ImGuiCol_TitleBgActive] = color_from_hex(0xf7f9fc);
	colors[ImGuiCol_TitleBgCollapsed] = color_from_hex(0xf7f9fc);
	colors[ImGuiCol_MenuBarBg] = color_from_hex(0xf7f9fc);
	colors[ImGuiCol_ScrollbarBg] = color_from_hex(0xecf1f5);
	colors[ImGuiCol_ScrollbarGrab] = color_from_hex(0xc8d4e0);
	colors[ImGuiCol_ScrollbarGrabHovered] = color_from_hex(0xb8c8d8);
	colors[ImGuiCol_ScrollbarGrabActive] = color_from_hex(0xa9bbcf);
	colors[ImGuiCol_CheckMark] = color_from_hex(0x4e82b5);
	colors[ImGuiCol_SliderGrab] = color_from_hex(0x4e82b5);
	colors[ImGuiCol_SliderGrabActive] = color_from_hex(0x3d6f9f);
	colors[ImGuiCol_Button] = color_from_hex(0xe7eef6);
	colors[ImGuiCol_ButtonHovered] = color_from_hex(0xdce7f3);
	colors[ImGuiCol_ButtonActive] = color_from_hex(0xd1deee);
	colors[ImGuiCol_Header] = color_from_hex(0xeaf0f6);
	colors[ImGuiCol_HeaderHovered] = color_from_hex(0xe2ebf4);
	colors[ImGuiCol_HeaderActive] = color_from_hex(0xd8e4f1);
	colors[ImGuiCol_Separator] = color_from_hex(0xd8dfe8);
	colors[ImGuiCol_SeparatorHovered] = color_from_hex(0xb9c9db);
	colors[ImGuiCol_SeparatorActive] = color_from_hex(0x9cb3c8);
	colors[ImGuiCol_ResizeGrip] = color_from_hex(0xb8c8d8, 0.40f);
	colors[ImGuiCol_ResizeGripHovered] = color_from_hex(0x9eb6cb, 0.72f);
	colors[ImGuiCol_ResizeGripActive] = color_from_hex(0x84a0bb, 0.90f);
	colors[ImGuiCol_Tab] = color_from_hex(0xeff3f7);
	colors[ImGuiCol_TabHovered] = color_from_hex(0xe5edf5);
	colors[ImGuiCol_TabActive] = color_from_hex(0xffffff);
	colors[ImGuiCol_TabUnfocused] = color_from_hex(0xebf0f5);
	colors[ImGuiCol_TabUnfocusedActive] = color_from_hex(0xf7f9fc);
	colors[ImGuiCol_PlotLines] = color_from_hex(0x6f95ba);
	colors[ImGuiCol_PlotLinesHovered] = color_from_hex(0x557da8);
	colors[ImGuiCol_PlotHistogram] = color_from_hex(0xa1b8cf);
	colors[ImGuiCol_PlotHistogramHovered] = color_from_hex(0x87a4c1);
	colors[ImGuiCol_TableHeaderBg] = color_from_hex(0xf1f5f9);
	colors[ImGuiCol_TableBorderStrong] = color_from_hex(0xd9e1ea);
	colors[ImGuiCol_TableBorderLight] = color_from_hex(0xe7edf3);
	colors[ImGuiCol_TableRowBg] = color_from_hex(0xfcfdff, 0.0f);
	colors[ImGuiCol_TableRowBgAlt] = color_from_hex(0xf5f8fb, 0.58f);
	colors[ImGuiCol_TextSelectedBg] = color_from_hex(0xb9d3eb, 0.42f);
	colors[ImGuiCol_DragDropTarget] = color_from_hex(0x5f8fbe, 0.95f);
	colors[ImGuiCol_NavHighlight] = color_from_hex(0x8ab0d2, 0.85f);
	colors[ImGuiCol_NavWindowingHighlight] = color_from_hex(0x8ab0d2, 0.85f);
	colors[ImGuiCol_NavWindowingDimBg] = color_from_hex(0x273544, 0.12f);
	colors[ImGuiCol_ModalWindowDimBg] = color_from_hex(0x273544, 0.20f);
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
		"/usr/share/fonts/TTF/OpenSans-Regular.ttf",
		"/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf",
		"/usr/share/fonts/noto/NotoSans-Regular.ttf",
		"/usr/share/fonts/noto/NotoSans-Light.ttf",
		"/usr/share/fonts/cantarell/Cantarell-VF.otf",
		"/usr/share/fonts/TTF/DejaVuSans.ttf",
		"/usr/share/fonts/liberation/LiberationSans-Regular.ttf"
	};
	const std::vector<std::string> title_candidates = {
		"/usr/share/fonts/TTF/OpenSans-SemiBold.ttf",
		"/usr/share/fonts/TTF/OpenSans-Bold.ttf",
		"/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf",
		"/usr/share/fonts/noto/NotoSans-Medium.ttf",
		"/usr/share/fonts/noto/NotoSans-Regular.ttf",
		"/usr/share/fonts/cantarell/Cantarell-VF.otf",
		"/usr/share/fonts/TTF/DejaVuSans.ttf",
		"/usr/share/fonts/liberation/LiberationSans-Regular.ttf"
	};
	const std::vector<std::string> section_candidates = title_candidates;
	const std::vector<std::string> mono_candidates = {
		"/usr/share/fonts/noto/NotoSansMono-Medium.ttf",
		"/usr/share/fonts/noto/NotoSansMono-Regular.ttf",
		"/usr/share/fonts/Adwaita/AdwaitaMono-Regular.ttf",
		"/usr/share/fonts/TTF/DejaVuSansMono.ttf"
	};

	app_fonts.body = try_load_font(io, sans_candidates, 14.25f);
	app_fonts.small = try_load_font(io, sans_candidates, 12.25f);
	app_fonts.section = try_load_font(io, section_candidates, 14.75f);
	app_fonts.title = try_load_font(io, title_candidates, 17.25f);
	app_fonts.mono = try_load_font(io, mono_candidates, 13.25f);

	if (app_fonts.body == nullptr) {
		app_fonts.body = io.Fonts->AddFontDefault();
	}
	if (app_fonts.small == nullptr) {
		app_fonts.small = app_fonts.body;
	}
	if (app_fonts.section == nullptr) {
		app_fonts.section = app_fonts.body;
	}
	if (app_fonts.title == nullptr) {
		app_fonts.title = app_fonts.section;
	}
	if (app_fonts.mono == nullptr) {
		app_fonts.mono = app_fonts.body;
	}
	io.FontDefault = app_fonts.body;
}

std::string backend_credit_plan_label(const BackendCreditSummary &credits)
{
	const std::string plan = trim_copy(credits.plan).empty() ? "free" : trim_copy(credits.plan);
	return plan + " plan";
}

std::string backend_credit_available_label(const BackendCreditSummary &credits)
{
	const int available = std::max(0, credits.available);
	return std::to_string(available) + (available == 1 ? " credit" : " credits");
}

std::string backend_credit_reserved_label(const BackendCreditSummary &credits)
{
	const int reserved = std::max(0, credits.reserved);
	return std::to_string(reserved) + " reserved";
}

std::string ai_mode_label(const std::string &mode)
{
	if (mode == "draft_grammar") {
		return "Draft Grammar";
	}
	if (mode == "repair_grammar") {
		return "Repair Grammar";
	}
	if (mode == "explain_grammar") {
		return "Explain Grammar";
	}
	if (mode == "tutor_next_step") {
		return "Tutor Next Step";
	}
	return "Active Helper";
}

std::string ai_mode_help_text(const std::string &mode)
{
	if (mode == "draft_grammar") {
		return "Use a design goal or brief. The backend returns a draft title, executable grammar, motifs, and next steps.";
	}
	if (mode == "repair_grammar") {
		return "Use this when the current grammar is broken or needs a safer rewrite around a concrete parser/runtime failure.";
	}
	if (mode == "explain_grammar") {
		return "Ask for explanation, review, or behavior analysis of the current grammar and selection.";
	}
	if (mode == "tutor_next_step") {
		return "Use this as a guided lesson. The backend returns diagnosis, one lesson, and a practical exercise.";
	}
	return "Use this as an editor-side helper. It answers directly and returns actions and warnings.";
}

bool ai_mode_prefers_prompt(const std::string &mode)
{
	return mode == "draft_grammar" || mode == "repair_grammar";
}

void clear_ai_assistant_state_internal(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return;
	}
	app_state->ai_assistant = AiAssistantSession{};
}

void begin_surface(const char *id, const ImVec2 &size, bool border = true, ImVec2 padding = ImVec2(8.0f, 8.0f))
{
	ImGui::PushStyleColor(ImGuiCol_ChildBg, color_from_hex(0xfcfdff));
	ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xd6dee8));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, padding);
	ImGui::BeginChild(id, size, border, ImGuiWindowFlags_NoScrollbar);
	ImGuiWindow *child_window = ImGui::GetCurrentWindow();
	if (child_window != nullptr) {
		const ImRect rect = child_window->Rect();
		child_window->DrawList->AddLine(ImVec2(rect.Min.x + 1.0f, rect.Min.y + 1.0f),
		                                ImVec2(rect.Max.x - 1.0f, rect.Min.y + 1.0f),
		                                ImGui::GetColorU32(color_from_hex(0xffffff, 0.65f)),
		                                1.0f);
	}
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
	ImGui::PushStyleColor(ImGuiCol_Separator, color_from_hex(0xd8e0e9));
	ImGui::Separator();
	ImGui::PopStyleColor();
}

void draw_section_heading(const char *label)
{
	ImFont *font = app_fonts.section != nullptr ? app_fonts.section : app_fonts.body;
	if (font != nullptr) {
		ImGui::PushFont(font);
	}
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(label);
	if (font != nullptr) {
		ImGui::PopFont();
	}
}

void draw_status_chip(const std::string &text,
                      unsigned int fill_rgb,
                      unsigned int text_rgb,
                      float fill_alpha = 0.10f,
                      float border_alpha = 0.22f)
{
	if (text.empty()) {
		return;
	}

	ImFont *chip_font = app_fonts.small != nullptr ? app_fonts.small : ImGui::GetFont();
	const float chip_font_size = chip_font != nullptr ? chip_font->FontSize : ImGui::GetFontSize();
	const bool pushed_font = chip_font != nullptr && chip_font != ImGui::GetFont();
	if (pushed_font) {
		ImGui::PushFont(chip_font);
	}
	const ImVec2 text_size = ImGui::CalcTextSize(text.c_str());
	if (pushed_font) {
		ImGui::PopFont();
	}
	const ImVec2 padding(8.0f, 2.5f);
	const ImVec2 pos = ImGui::GetCursorScreenPos();
	const ImVec2 size(text_size.x + padding.x * 2.0f, text_size.y + padding.y * 2.0f);
	ImGui::Dummy(size);

	const ImRect rect(pos, ImVec2(pos.x + size.x, pos.y + size.y));
	ImDrawList *draw_list = ImGui::GetWindowDrawList();
	const float rounding = size.y * 0.5f;
	draw_list->AddRectFilled(rect.Min,
	                         rect.Max,
	                         ImGui::GetColorU32(color_from_hex(fill_rgb, fill_alpha)),
	                         rounding);
	draw_list->AddRect(rect.Min,
	                   rect.Max,
	                   ImGui::GetColorU32(color_from_hex(fill_rgb, border_alpha)),
	                   rounding,
	                   0,
	                   1.0f);
	draw_list->AddText(chip_font,
	                   chip_font_size,
	                   ImVec2(rect.Min.x + padding.x,
	                          rect.Min.y + std::floor((size.y - chip_font_size) * 0.5f) - 0.5f),
	                   ImGui::GetColorU32(color_from_hex(text_rgb)),
	                   text.c_str());
}

void push_primary_action_button_style()
{
	ImGui::PushStyleColor(ImGuiCol_Button, color_from_hex(0x4b7aa9));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color_from_hex(0x416f9d));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, color_from_hex(0x375f88));
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0xf8fbff));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
}

void pop_primary_action_button_style()
{
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(4);
}

void draw_startup_loader_window(const std::string &detail,
                                float progress,
                                int completed_stages,
                                int total_stages)
{
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	if (viewport == nullptr) {
		return;
	}

	const float width = std::clamp(viewport->Size.x - 48.0f, 360.0f, 560.0f);
	ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::SetNextWindowSize(ImVec2(width, 0.0f), ImGuiCond_Always);

	ImGui::PushStyleColor(ImGuiCol_WindowBg, color_from_hex(0xfffcf9, 0.985f));
	ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xd5dde7));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, color_from_hex(0xecf2f8));
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, color_from_hex(0x7aa1c6));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 18.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(22.0f, 20.0f));

	ImGui::Begin("StartupLoaderWindow",
	             nullptr,
	             ImGuiWindowFlags_NoDecoration |
	                 ImGuiWindowFlags_NoMove |
	                 ImGuiWindowFlags_NoSavedSettings);

	if (app_fonts.title != nullptr) {
		ImGui::PushFont(app_fonts.title);
	}
	ImGui::TextUnformatted("Launching Progen3d");
	if (app_fonts.title != nullptr) {
		ImGui::PopFont();
	}

	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x8f8882));
	if (completed_stages >= total_stages && total_stages > 0) {
		ImGui::TextUnformatted("Startup complete");
	} else if (total_stages > 0) {
		ImGui::Text("Step %d of %d", std::min(completed_stages + 1, total_stages), total_stages);
	} else {
		ImGui::TextUnformatted("Starting up");
	}
	ImGui::PopStyleColor();

	draw_divider();

	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x6f6862));
	ImGui::TextWrapped("%s", detail.c_str());
	ImGui::PopStyleColor();

	const int percent = static_cast<int>(std::round(std::clamp(progress, 0.0f, 1.0f) * 100.0f));
	ImGui::Dummy(ImVec2(0.0f, 10.0f));
	ImGui::ProgressBar(std::clamp(progress, 0.0f, 1.0f), ImVec2(-FLT_MIN, 14.0f));
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
	ImGui::Text("%d%%", percent);
	ImGui::PopStyleColor();

	ImGui::End();
	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor(4);
}

bool render_startup_loader_frame(GLFWwindow *window,
                                 const std::string &detail,
                                 float progress,
                                 int completed_stages,
                                 int total_stages)
{
	if (window == nullptr) {
		return false;
	}

	glfwPollEvents();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	draw_startup_loader_window(detail, progress, completed_stages, total_stages);

	ImGui::Render();
	int display_w = 0;
	int display_h = 0;
	glfwGetFramebufferSize(window, &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);
	glClearColor(0.949f, 0.964f, 0.980f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwSwapBuffers(window);
	return !glfwWindowShouldClose(window);
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
	ImGui::PushStyleColor(ImGuiCol_Button, color_from_hex(0xe7edf5));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color_from_hex(0xdbe6f2));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, color_from_hex(0xcad8e9));
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
	application_log.append(std::move(message));
}

void clear_console_messages()
{
	application_log.clear();
	debug_state_messages.clear();
}

void clear_variable_pool()
{
	if (grammar != nullptr) {
		clearGrammarRuntimeVariableSnapshots(grammar);
	}
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
	smart_editor_state = GrammarEditorInteractionState{};
}

void destroy_grammar()
{
	if (grammar != nullptr) {
		clearGrammarRuntimeVariableSnapshots(grammar);
	}
	current_scene_snapshot().reset();
	grammar = nullptr;
}

struct MaterialSwatch {
	float r = 1.0f;
	float g = 1.0f;
	float b = 1.0f;
};

struct ParsedMaterialSpec {
	std::string name;
	std::string family = "blend";
	std::string description;
	std::vector<std::string> color_names;
	std::vector<std::string> descriptors;
	std::vector<MaterialSwatch> palette;
	std::string wood_species;
	float opacity = 1.0f;
	float reflectance = 0.28f;
	float smoothness = 0.45f;
	float height_scale = 0.04f;
	float emission_strength = 0.0f;
	float uv_rotation_speed = 0.0f;
	float uv_activity = 0.0f;
	float pulse_strength = 0.0f;
	float normal_strength = 1.0f;
	float transmission = 0.0f;
	float index_of_refraction = 1.5f;
	float thickness = 0.01f;
	float anisotropic = 0.0f;
	float sheen = 0.0f;
	float clearcoat = 0.0f;
	float clearcoat_roughness = 0.0f;
	MaterialSwatch attenuation_color{1.0f, 1.0f, 1.0f};
	bool two_tone = false;
	bool vertical_pattern = false;
	bool emphasize_veins = false;
	uint32_t seed = 0;
};

enum class MaterialLexemeKind {
	Color,
	Family,
	Opacity,
	Finish
};

struct MaterialLexeme {
	std::string_view token;
	MaterialLexemeKind kind = MaterialLexemeKind::Color;
	MaterialSwatch swatch{};
};

struct MaterialTokenMatch {
	std::size_t position = 0;
	const MaterialLexeme *lexeme = nullptr;
};

struct MaterialAutocompleteSuggestion {
	std::string label;
	std::string replacement;
	MaterialLexemeKind kind = MaterialLexemeKind::Color;
	MaterialSwatch swatch{};
};

struct MaterialAutocomplete {
	int line = -1;
	int replace_start_column = 0;
	int replace_end_column = 0;
	std::string prefix;
	bool active = false;
	bool append_trailing_space = false;
	std::vector<MaterialAutocompleteSuggestion> suggestions;
};

struct MaterialAutocompletePrefixState {
	std::vector<const MaterialLexeme *> matches;
	std::string trailing_fragment;
	int color_count = 0;
	bool has_opacity = false;
	bool has_family = false;
	bool has_finish = false;
};

float clamp01(float value)
{
	return std::clamp(value, 0.0f, 1.0f);
}

float lerp_scalar(float a, float b, float t)
{
	return a + (b - a) * t;
}

float smooth_curve(float value)
{
	value = clamp01(value);
	return value * value * (3.0f - 2.0f * value);
}

MaterialSwatch make_swatch(float r, float g, float b)
{
	return MaterialSwatch{r, g, b};
}

MaterialSwatch mix_swatch(const MaterialSwatch &a, const MaterialSwatch &b, float t)
{
	return make_swatch(lerp_scalar(a.r, b.r, t),
	                   lerp_scalar(a.g, b.g, t),
	                   lerp_scalar(a.b, b.b, t));
}

MaterialSwatch scale_swatch(const MaterialSwatch &color, float scale)
{
	return make_swatch(clamp01(color.r * scale),
	                   clamp01(color.g * scale),
	                   clamp01(color.b * scale));
}

MaterialSwatch lighten_swatch(const MaterialSwatch &color, float amount)
{
	return mix_swatch(color, make_swatch(1.0f, 1.0f, 1.0f), clamp01(amount));
}

MaterialSwatch darken_swatch(const MaterialSwatch &color, float amount)
{
	return mix_swatch(color, make_swatch(0.0f, 0.0f, 0.0f), clamp01(amount));
}

uint32_t stable_hash(const std::string &text)
{
	uint32_t hash = 2166136261u;
	for (unsigned char character : text) {
		hash ^= static_cast<uint32_t>(character);
		hash *= 16777619u;
	}
	return hash;
}

float lattice_noise(int x, int y, uint32_t seed)
{
	uint32_t value = seed;
	value ^= static_cast<uint32_t>(x) * 374761393u;
	value ^= static_cast<uint32_t>(y) * 668265263u;
	value = (value ^ (value >> 13u)) * 1274126177u;
	value ^= value >> 16u;
	return static_cast<float>(value & 0x00ffffffu) / static_cast<float>(0x01000000u);
}

float value_noise(float x, float y, uint32_t seed)
{
	const int x0 = static_cast<int>(std::floor(x));
	const int y0 = static_cast<int>(std::floor(y));
	const int x1 = x0 + 1;
	const int y1 = y0 + 1;
	const float tx = smooth_curve(x - static_cast<float>(x0));
	const float ty = smooth_curve(y - static_cast<float>(y0));

	const float n00 = lattice_noise(x0, y0, seed);
	const float n10 = lattice_noise(x1, y0, seed);
	const float n01 = lattice_noise(x0, y1, seed);
	const float n11 = lattice_noise(x1, y1, seed);
	const float nx0 = lerp_scalar(n00, n10, tx);
	const float nx1 = lerp_scalar(n01, n11, tx);
	return lerp_scalar(nx0, nx1, ty);
}

float fractal_noise(float x, float y, uint32_t seed, int octaves = 5)
{
	float amplitude = 0.5f;
	float frequency = 1.0f;
	float total = 0.0f;
	float normalization = 0.0f;
	for (int octave = 0; octave < octaves; ++octave) {
		total += value_noise(x * frequency, y * frequency, seed + static_cast<uint32_t>(octave * 977u)) * amplitude;
		normalization += amplitude;
		amplitude *= 0.5f;
		frequency *= 2.0f;
	}
	return normalization > 0.0f ? total / normalization : 0.0f;
}

std::string join_strings(const std::vector<std::string> &parts)
{
	std::ostringstream stream;
	for (std::size_t index = 0; index < parts.size(); ++index) {
		if (index > 0) {
			stream << ", ";
		}
		stream << parts[index];
	}
	return stream.str();
}

std::vector<std::string> split_string_non_empty(const std::string &value, char delimiter)
{
	std::vector<std::string> tokens;
	std::string current;
	for (char character : value) {
		if (character == delimiter) {
			if (!current.empty()) {
				tokens.push_back(current);
				current.clear();
			}
			continue;
		}
		current.push_back(character);
	}
	if (!current.empty()) {
		tokens.push_back(current);
	}
	return tokens;
}

std::vector<std::string> split_string_preserve_empty(const std::string &value, char delimiter)
{
	std::vector<std::string> tokens;
	std::string current;
	for (char character : value) {
		if (character == delimiter) {
			tokens.push_back(current);
			current.clear();
			continue;
		}
		current.push_back(character);
	}
	tokens.push_back(current);
	return tokens;
}

std::string join_string_tokens(const std::vector<std::string> &tokens, char delimiter)
{
	std::ostringstream stream;
	for (std::size_t index = 0; index < tokens.size(); ++index) {
		if (index > 0) {
			stream << delimiter;
		}
		stream << tokens[index];
	}
	return stream.str();
}

void sort_and_unique_strings(std::vector<std::string> *values)
{
	if (values == nullptr) {
		return;
	}
	std::sort(values->begin(), values->end());
	values->erase(std::unique(values->begin(), values->end()), values->end());
}

template <std::size_t N>
std::vector<MaterialTokenMatch> scan_material_lexemes(const std::string &name,
                                                      const std::array<MaterialLexeme, N> &lexemes)
{
	std::vector<MaterialTokenMatch> matches;
	for (std::size_t position = 0; position < name.size();) {
		const MaterialLexeme *best_match = nullptr;
		for (const MaterialLexeme &lexeme : lexemes) {
			if (lexeme.token.empty() ||
			    position + lexeme.token.size() > name.size() ||
			    name.compare(position, lexeme.token.size(), lexeme.token) != 0) {
				continue;
			}
			if (best_match == nullptr || lexeme.token.size() > best_match->token.size()) {
				best_match = &lexeme;
			}
		}
		if (best_match != nullptr) {
			matches.push_back({position, best_match});
			position += best_match->token.size();
		} else {
			++position;
		}
	}
	return matches;
}

bool material_matches_token(const std::vector<MaterialTokenMatch> &matches, std::string_view token)
{
	for (const MaterialTokenMatch &match : matches) {
		if (match.lexeme != nullptr && match.lexeme->token == token) {
			return true;
		}
	}
	return false;
}

const std::array<MaterialLexeme, 78> &material_lexemes()
{
	static const std::array<MaterialLexeme, 78> lexemes = {{
		{"transparent", MaterialLexemeKind::Opacity, {}},
		{"translucent", MaterialLexemeKind::Opacity, {}},
		{"emissive", MaterialLexemeKind::Finish, {}},
		{"glowing", MaterialLexemeKind::Finish, {}},
		{"active", MaterialLexemeKind::Finish, {}},
		{"animated", MaterialLexemeKind::Finish, {}},
		{"rotating", MaterialLexemeKind::Finish, {}},
		{"reflective", MaterialLexemeKind::Finish, {}},
		{"weathered", MaterialLexemeKind::Finish, {}},
		{"polished", MaterialLexemeKind::Finish, {}},
		{"mirror", MaterialLexemeKind::Finish, {}},
		{"concrete", MaterialLexemeKind::Family, {}},
		{"ceramic", MaterialLexemeKind::Family, {}},
		{"plastic", MaterialLexemeKind::Family, {}},
		{"plaster", MaterialLexemeKind::Family, {}},
		{"grass", MaterialLexemeKind::Family, {}},
		{"brick", MaterialLexemeKind::Family, {}},
		{"neon", MaterialLexemeKind::Family, {}},
		{"floral", MaterialLexemeKind::Family, {}},
		{"boards", MaterialLexemeKind::Family, {}},
		{"orange", MaterialLexemeKind::Color, make_swatch(0.84f, 0.48f, 0.24f)},
		{"purple", MaterialLexemeKind::Color, make_swatch(0.58f, 0.42f, 0.76f)},
		{"silver", MaterialLexemeKind::Color, make_swatch(0.76f, 0.78f, 0.81f)},
		{"yellow", MaterialLexemeKind::Color, make_swatch(0.88f, 0.78f, 0.32f)},
		{"magenta", MaterialLexemeKind::Color, make_swatch(0.88f, 0.26f, 0.86f)},
		{"lime", MaterialLexemeKind::Color, make_swatch(0.62f, 0.94f, 0.24f)},
		{"violet", MaterialLexemeKind::Color, make_swatch(0.50f, 0.38f, 0.90f)},
		{"twotone", MaterialLexemeKind::Finish, {}},
		{"duotone", MaterialLexemeKind::Finish, {}},
		{"horizontal", MaterialLexemeKind::Finish, {}},
		{"vertical", MaterialLexemeKind::Finish, {}},
		{"veins", MaterialLexemeKind::Finish, {}},
		{"veined", MaterialLexemeKind::Finish, {}},
		{"oak", MaterialLexemeKind::Finish, {}},
		{"walnut", MaterialLexemeKind::Finish, {}},
		{"pine", MaterialLexemeKind::Finish, {}},
		{"cedar", MaterialLexemeKind::Finish, {}},
		{"birch", MaterialLexemeKind::Finish, {}},
		{"smooth", MaterialLexemeKind::Finish, {}},
		{"glossy", MaterialLexemeKind::Finish, {}},
		{"opaque", MaterialLexemeKind::Opacity, {}},
		{"shiny", MaterialLexemeKind::Finish, {}},
		{"rough", MaterialLexemeKind::Finish, {}},
		{"smoky", MaterialLexemeKind::Opacity, {}},
		{"white", MaterialLexemeKind::Color, make_swatch(0.94f, 0.95f, 0.96f)},
		{"green", MaterialLexemeKind::Color, make_swatch(0.35f, 0.62f, 0.39f)},
		{"black", MaterialLexemeKind::Color, make_swatch(0.16f, 0.18f, 0.20f)},
		{"chrome", MaterialLexemeKind::Color, make_swatch(0.84f, 0.87f, 0.90f)},
		{"copper", MaterialLexemeKind::Color, make_swatch(0.74f, 0.47f, 0.33f)},
		{"beige", MaterialLexemeKind::Color, make_swatch(0.81f, 0.75f, 0.63f)},
		{"glass", MaterialLexemeKind::Family, {}},
		{"marble", MaterialLexemeKind::Family, {}},
		{"stone", MaterialLexemeKind::Family, {}},
		{"metal", MaterialLexemeKind::Family, {}},
		{"paint", MaterialLexemeKind::Family, {}},
		{"brown", MaterialLexemeKind::Color, make_swatch(0.48f, 0.34f, 0.22f)},
		{"blue", MaterialLexemeKind::Color, make_swatch(0.28f, 0.50f, 0.84f)},
		{"grey", MaterialLexemeKind::Color, make_swatch(0.58f, 0.60f, 0.63f)},
		{"gray", MaterialLexemeKind::Color, make_swatch(0.58f, 0.60f, 0.63f)},
		{"gold", MaterialLexemeKind::Color, make_swatch(0.82f, 0.70f, 0.33f)},
		{"wood", MaterialLexemeKind::Family, {}},
		{"rock", MaterialLexemeKind::Family, {}},
		{"tile", MaterialLexemeKind::Family, {}},
		{"amber", MaterialLexemeKind::Color, make_swatch(0.92f, 0.66f, 0.22f)},
		{"teal", MaterialLexemeKind::Color, make_swatch(0.24f, 0.63f, 0.67f)},
		{"cyan", MaterialLexemeKind::Color, make_swatch(0.33f, 0.71f, 0.81f)},
		{"pink", MaterialLexemeKind::Color, make_swatch(0.87f, 0.63f, 0.73f)},
		{"red", MaterialLexemeKind::Color, make_swatch(0.78f, 0.28f, 0.24f)},
			{"aged", MaterialLexemeKind::Finish, {}},
			{"matte", MaterialLexemeKind::Finish, {}},
			{"satin", MaterialLexemeKind::Finish, {}},
			{"frosted", MaterialLexemeKind::Finish, {}},
			{"crystal", MaterialLexemeKind::Finish, {}},
			{"brushed", MaterialLexemeKind::Finish, {}},
			{"velvet", MaterialLexemeKind::Finish, {}},
			{"clearcoat", MaterialLexemeKind::Finish, {}},
			{"dull", MaterialLexemeKind::Finish, {}}
	}};
	return lexemes;
}

std::string normalize_material_autocomplete_text(const std::string &text)
{
	std::string normalized;
	normalized.reserve(text.size());
	for (unsigned char character : text) {
		if (std::isalnum(character) != 0) {
			normalized.push_back(static_cast<char>(std::tolower(character)));
		}
	}
	return normalized;
}

std::string format_material_lexeme_label(std::string_view token)
{
	if (token.empty()) {
		return "";
	}
	std::string label(token);
	label[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(label[0])));
	for (std::size_t index = 1; index < label.size(); ++index) {
		label[index] = static_cast<char>(std::tolower(static_cast<unsigned char>(label[index])));
	}
	return label;
}

MaterialAutocompletePrefixState analyze_material_autocomplete_prefix(const std::string &prefix)
{
	MaterialAutocompletePrefixState state;
	const std::string normalized_prefix = normalize_material_autocomplete_text(prefix);
	const auto &lexemes = material_lexemes();
	std::size_t position = 0;
	while (position < normalized_prefix.size()) {
		const MaterialLexeme *best_match = nullptr;
		for (const MaterialLexeme &lexeme : lexemes) {
			if (position + lexeme.token.size() > normalized_prefix.size() ||
			    normalized_prefix.compare(position, lexeme.token.size(), lexeme.token) != 0) {
				continue;
			}
			if (best_match == nullptr || lexeme.token.size() > best_match->token.size()) {
				best_match = &lexeme;
			}
		}
		if (best_match == nullptr) {
			state.trailing_fragment = normalized_prefix.substr(position);
			break;
		}
		state.matches.push_back(best_match);
		if (best_match->kind == MaterialLexemeKind::Color) {
			++state.color_count;
		} else if (best_match->kind == MaterialLexemeKind::Opacity) {
			state.has_opacity = true;
		} else if (best_match->kind == MaterialLexemeKind::Family) {
			state.has_family = true;
		} else if (best_match->kind == MaterialLexemeKind::Finish) {
			state.has_finish = true;
		}
		position += best_match->token.size();
	}
	return state;
}

bool material_prefix_contains_lexeme(const MaterialAutocompletePrefixState &state, std::string_view token)
{
	for (const MaterialLexeme *lexeme : state.matches) {
		if (lexeme != nullptr && lexeme->token == token) {
			return true;
		}
	}
	return false;
}

std::vector<MaterialLexemeKind> ordered_material_autocomplete_kinds(const MaterialAutocompletePrefixState &state)
{
	if (state.color_count <= 0) {
		return {MaterialLexemeKind::Color,
		        MaterialLexemeKind::Opacity,
		        MaterialLexemeKind::Family,
		        MaterialLexemeKind::Finish};
	}
	if (!state.has_opacity) {
		return {MaterialLexemeKind::Opacity,
		        MaterialLexemeKind::Family,
		        MaterialLexemeKind::Finish,
		        MaterialLexemeKind::Color};
	}
	if (!state.has_family) {
		return {MaterialLexemeKind::Family,
		        MaterialLexemeKind::Finish,
		        MaterialLexemeKind::Color,
		        MaterialLexemeKind::Opacity};
	}
	return {MaterialLexemeKind::Finish,
	        MaterialLexemeKind::Color,
	        MaterialLexemeKind::Opacity,
	        MaterialLexemeKind::Family};
}

const char *label_for_material_autocomplete_kind(MaterialLexemeKind kind)
{
	switch (kind) {
	case MaterialLexemeKind::Color:
		return "Color";
	case MaterialLexemeKind::Opacity:
		return "Transparency";
	case MaterialLexemeKind::Family:
		return "Material";
	case MaterialLexemeKind::Finish:
		return "Finish";
	}
	return "Material";
}

std::string build_material_autocomplete_replacement(const MaterialAutocompletePrefixState &state,
                                                    const MaterialLexeme &candidate)
{
	std::string replacement;
	for (const MaterialLexeme *lexeme : state.matches) {
		if (lexeme == nullptr) {
			continue;
		}
		replacement += format_material_lexeme_label(lexeme->token);
	}
	replacement += format_material_lexeme_label(candidate.token);
	return replacement;
}

ParsedMaterialSpec parse_material_name(const std::string &material_name)
{
	ParsedMaterialSpec spec;
	spec.name = material_name.empty() ? "softwhitematteplaster" : material_name;
	spec.seed = stable_hash(spec.name);
	const auto &lexemes = material_lexemes();
	const std::vector<MaterialTokenMatch> matches = scan_material_lexemes(spec.name, lexemes);
	for (const MaterialTokenMatch &match : matches) {
		if (spec.color_names.size() >= 3) {
			break;
		}
		if (match.lexeme == nullptr || match.lexeme->kind != MaterialLexemeKind::Color) {
			continue;
		}
		const std::string color_name(match.lexeme->token);
		if (std::find(spec.color_names.begin(), spec.color_names.end(), color_name) == spec.color_names.end()) {
			spec.color_names.push_back(color_name);
			spec.palette.push_back(match.lexeme->swatch);
		}
	}
	const bool has_explicit_colors = !spec.palette.empty();
	const bool wants_boards = material_matches_token(matches, "boards");
	const bool wants_floral = material_matches_token(matches, "floral");
	const bool wants_horizontal = material_matches_token(matches, "horizontal");
	spec.vertical_pattern = material_matches_token(matches, "vertical");
	if (wants_horizontal) {
		spec.vertical_pattern = false;
	}
	spec.two_tone = material_matches_token(matches, "twotone") ||
	                material_matches_token(matches, "duotone");
	spec.emphasize_veins = material_matches_token(matches, "veins") ||
	                       material_matches_token(matches, "veined");

	auto set_palette = [&](const std::vector<std::string> &labels,
	                      const MaterialSwatch &first,
	                      const MaterialSwatch &second,
	                      const MaterialSwatch &third) {
		spec.color_names = labels;
		spec.palette = {first, second, third};
	};

	if (material_matches_token(matches, "oak")) {
		spec.wood_species = "oak";
	} else if (material_matches_token(matches, "walnut")) {
		spec.wood_species = "walnut";
	} else if (material_matches_token(matches, "pine")) {
		spec.wood_species = "pine";
	} else if (material_matches_token(matches, "cedar")) {
		spec.wood_species = "cedar";
	} else if (material_matches_token(matches, "birch")) {
		spec.wood_species = "birch";
	}

	if (!has_explicit_colors) {
		if (spec.wood_species == "oak") {
			set_palette({"oak", "natural"},
			            make_swatch(0.74f, 0.56f, 0.34f),
			            make_swatch(0.48f, 0.35f, 0.19f),
			            make_swatch(0.88f, 0.74f, 0.50f));
		} else if (spec.wood_species == "walnut") {
			set_palette({"walnut", "natural"},
			            make_swatch(0.42f, 0.28f, 0.18f),
			            make_swatch(0.24f, 0.16f, 0.10f),
			            make_swatch(0.58f, 0.40f, 0.24f));
		} else if (spec.wood_species == "pine") {
			set_palette({"pine", "natural"},
			            make_swatch(0.84f, 0.72f, 0.46f),
			            make_swatch(0.64f, 0.50f, 0.27f),
			            make_swatch(0.94f, 0.85f, 0.64f));
		} else if (spec.wood_species == "cedar") {
			set_palette({"cedar", "natural"},
			            make_swatch(0.72f, 0.40f, 0.24f),
			            make_swatch(0.46f, 0.23f, 0.14f),
			            make_swatch(0.84f, 0.58f, 0.38f));
		} else if (spec.wood_species == "birch") {
			set_palette({"birch", "natural"},
			            make_swatch(0.88f, 0.80f, 0.62f),
			            make_swatch(0.68f, 0.58f, 0.38f),
			            make_swatch(0.96f, 0.91f, 0.74f));
		} else if (wants_floral) {
			set_palette({"rose", "sage"},
			            make_swatch(0.86f, 0.60f, 0.70f),
			            make_swatch(0.92f, 0.89f, 0.82f),
			            make_swatch(0.48f, 0.66f, 0.46f));
		}
	}

	if (spec.palette.empty()) {
		spec.color_names = {"white", "grey"};
		spec.palette = {make_swatch(0.90f, 0.91f, 0.92f),
		                make_swatch(0.60f, 0.62f, 0.66f)};
	}
	if (spec.palette.size() == 1) {
		spec.palette.push_back(lighten_swatch(spec.palette.front(), 0.22f));
	}

	if (wants_floral) {
		spec.family = "floral";
		spec.reflectance = 0.20f;
		spec.smoothness = 0.40f;
		spec.height_scale = 0.050f;
	} else if (material_matches_token(matches, "metal")) {
		spec.family = "metal";
		spec.reflectance = 0.88f;
		spec.smoothness = 0.78f;
		spec.height_scale = 0.030f;
		} else if (material_matches_token(matches, "glass")) {
			spec.family = "glass";
			spec.opacity = 0.38f;
			spec.reflectance = 0.82f;
			spec.smoothness = 0.95f;
			spec.height_scale = 0.010f;
			spec.transmission = 0.94f;
			spec.index_of_refraction = 1.50f;
			spec.thickness = 0.018f;
			spec.clearcoat = 0.32f;
			spec.clearcoat_roughness = 0.05f;
	} else if (material_matches_token(matches, "neon")) {
		spec.family = "neon";
		spec.reflectance = 0.58f;
		spec.smoothness = 0.94f;
		spec.height_scale = 0.025f;
		spec.emission_strength = 1.15f;
	} else if (material_matches_token(matches, "grass")) {
		spec.family = "grass";
		spec.reflectance = 0.10f;
		spec.smoothness = 0.18f;
		spec.height_scale = 0.075f;
	} else if (material_matches_token(matches, "brick")) {
		spec.family = "brick";
		spec.reflectance = 0.12f;
		spec.smoothness = 0.26f;
		spec.height_scale = 0.085f;
	} else if (wants_boards) {
		spec.family = "boards";
		spec.reflectance = 0.22f;
		spec.smoothness = 0.40f;
		spec.height_scale = 0.060f;
	} else if (material_matches_token(matches, "wood") || !spec.wood_species.empty()) {
		spec.family = "wood";
		spec.reflectance = 0.20f;
		spec.smoothness = 0.38f;
		spec.height_scale = 0.055f;
	} else if (material_matches_token(matches, "marble")) {
		spec.family = "marble";
		spec.reflectance = 0.30f;
		spec.smoothness = 0.72f;
		spec.height_scale = 0.032f;
	} else if (material_matches_token(matches, "stone") || material_matches_token(matches, "rock")) {
		spec.family = "stone";
		spec.reflectance = 0.22f;
		spec.smoothness = 0.44f;
		spec.height_scale = 0.065f;
	} else if (material_matches_token(matches, "concrete")) {
		spec.family = "concrete";
		spec.reflectance = 0.16f;
		spec.smoothness = 0.32f;
		spec.height_scale = 0.070f;
	} else if (material_matches_token(matches, "ceramic") || material_matches_token(matches, "tile")) {
		spec.family = "ceramic";
		spec.reflectance = 0.46f;
		spec.smoothness = 0.84f;
		spec.height_scale = 0.040f;
	} else if (material_matches_token(matches, "plastic")) {
		spec.family = "plastic";
		spec.reflectance = 0.40f;
		spec.smoothness = 0.76f;
		spec.height_scale = 0.024f;
	} else if (material_matches_token(matches, "paint") || material_matches_token(matches, "plaster")) {
		spec.family = "paint";
		spec.reflectance = 0.26f;
		spec.smoothness = 0.58f;
		spec.height_scale = 0.028f;
	}

	if (material_matches_token(matches, "transparent")) {
		spec.opacity = 0.24f;
	}
	if (material_matches_token(matches, "translucent")) {
		spec.opacity = 0.45f;
	}
	if (material_matches_token(matches, "smoky")) {
		spec.opacity = std::min(spec.opacity, 0.68f);
		spec.attenuation_color = make_swatch(0.54f, 0.60f, 0.66f);
		spec.thickness = std::max(spec.thickness, 0.045f);
	}
	if (material_matches_token(matches, "opaque")) {
		spec.opacity = 1.0f;
	}

	if (material_matches_token(matches, "shiny") ||
	    material_matches_token(matches, "glossy") ||
	    material_matches_token(matches, "polished") ||
	    material_matches_token(matches, "reflective")) {
		spec.reflectance = std::max(spec.reflectance, 0.78f);
		spec.smoothness = std::max(spec.smoothness, 0.86f);
	}
	if (material_matches_token(matches, "mirror")) {
		spec.reflectance = std::max(spec.reflectance, 0.96f);
		spec.smoothness = std::max(spec.smoothness, 0.98f);
		spec.height_scale = std::min(spec.height_scale, 0.014f);
	}
	if (material_matches_token(matches, "frosted")) {
		spec.smoothness = std::min(spec.smoothness, 0.42f);
		spec.transmission = std::max(spec.transmission, 0.78f);
		spec.opacity = std::max(spec.opacity, 0.52f);
		spec.normal_strength = std::max(spec.normal_strength, 1.35f);
	}
	if (material_matches_token(matches, "crystal")) {
		spec.family = "glass";
		spec.smoothness = 0.99f;
		spec.reflectance = 0.90f;
		spec.transmission = 0.98f;
		spec.index_of_refraction = 1.52f;
		spec.opacity = 0.30f;
	}
	if (material_matches_token(matches, "brushed")) {
		spec.anisotropic = 0.72f;
		spec.smoothness = std::min(spec.smoothness, 0.66f);
	}
	if (material_matches_token(matches, "velvet")) {
		spec.sheen = 0.82f;
		spec.smoothness = std::min(spec.smoothness, 0.38f);
	}
	if (material_matches_token(matches, "clearcoat")) {
		spec.clearcoat = 1.0f;
		spec.clearcoat_roughness = 0.08f;
	}
	if (material_matches_token(matches, "smooth") || material_matches_token(matches, "satin")) {
		spec.smoothness = std::max(spec.smoothness, 0.78f);
	}
	if (material_matches_token(matches, "rough") ||
	    material_matches_token(matches, "aged") ||
	    material_matches_token(matches, "weathered")) {
		spec.smoothness = std::min(spec.smoothness, 0.28f);
	}
	if (material_matches_token(matches, "matte") || material_matches_token(matches, "dull")) {
		spec.reflectance = std::min(spec.reflectance, 0.22f);
		spec.smoothness = std::min(spec.smoothness, 0.42f);
	}
	if (material_matches_token(matches, "glowing") ||
	    material_matches_token(matches, "emissive")) {
		spec.emission_strength = std::max(spec.emission_strength, 0.95f);
		spec.smoothness = std::max(spec.smoothness, 0.72f);
	}
	if (material_matches_token(matches, "rotating")) {
		spec.uv_rotation_speed = std::max(spec.uv_rotation_speed, 0.70f);
		spec.pulse_strength = std::max(spec.pulse_strength, 0.12f);
	}
	if (material_matches_token(matches, "animated") ||
	    material_matches_token(matches, "active")) {
		spec.uv_activity = std::max(spec.uv_activity, 1.0f);
		spec.pulse_strength = std::max(spec.pulse_strength, 0.34f);
	}
	if (spec.family == "neon") {
		spec.emission_strength = std::max(spec.emission_strength,
		                                  material_matches_token(matches, "glowing") ? 1.85f : 1.25f);
	}
	if (spec.family == "glass") {
		spec.reflectance = std::max(spec.reflectance, 0.82f);
	}
	if (spec.family == "boards" && spec.wood_species.empty()) {
		spec.wood_species = "oak";
	}
	if (spec.two_tone) {
		spec.smoothness = std::max(spec.smoothness, 0.44f);
	}
	if (spec.emphasize_veins) {
		spec.height_scale = std::max(spec.height_scale, 0.050f);
	}
	if (!spec.wood_species.empty()) {
		spec.descriptors.push_back(spec.wood_species);
	}
	if (wants_horizontal || spec.vertical_pattern) {
		spec.descriptors.push_back(spec.vertical_pattern ? "vertical" : "horizontal");
	}
	if (spec.two_tone) {
		spec.descriptors.push_back("two tone");
	}
	if (spec.emphasize_veins) {
		spec.descriptors.push_back("veins");
	}
	if (spec.uv_rotation_speed > 0.0f) {
		spec.descriptors.push_back("rotating");
	}
	if (spec.uv_activity > 0.0f) {
		spec.descriptors.push_back("active");
	}
	sort_and_unique_strings(&spec.descriptors);

	spec.opacity = clamp01(spec.opacity);
	spec.reflectance = clamp01(spec.reflectance);
	spec.smoothness = clamp01(spec.smoothness);
	spec.height_scale = std::clamp(spec.height_scale, 0.0f, 0.16f);
	spec.emission_strength = std::clamp(spec.emission_strength, 0.0f, 2.5f);
	spec.pulse_strength = std::clamp(spec.pulse_strength, 0.0f, 1.0f);
	spec.normal_strength = std::clamp(spec.normal_strength, 0.0f, 2.0f);
	spec.transmission = clamp01(spec.transmission);
	spec.index_of_refraction = std::clamp(spec.index_of_refraction, 1.0f, 2.5f);
	spec.thickness = std::clamp(spec.thickness, 0.0f, 1.0f);
	spec.anisotropic = clamp01(spec.anisotropic);
	spec.sheen = clamp01(spec.sheen);
	spec.clearcoat = clamp01(spec.clearcoat);
	spec.clearcoat_roughness = clamp01(spec.clearcoat_roughness);

	std::ostringstream description;
	description << spec.family;
	if (!spec.descriptors.empty()) {
		description << " • " << join_strings(spec.descriptors);
	}
	description << " • " << join_strings(spec.color_names)
	            << " • opacity " << std::fixed << std::setprecision(2) << spec.opacity
	            << " • reflectance " << spec.reflectance
	            << " • smoothness " << spec.smoothness
		            << " • height " << spec.height_scale
		            << " • glow " << spec.emission_strength
		            << " • transmission " << spec.transmission
		            << " • ior " << spec.index_of_refraction;
	spec.description = description.str();
	return spec;
}

GLuint upload_rgba_texture(const unsigned char *pixels,
	                       int width,
	                       int height,
	                       bool color_texture = false)
{
	if (pixels == nullptr || width <= 0 || height <= 0) {
		return 0;
	}

	auto configure_texture_antialiasing = []() {
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if (!progen3d_texture_anisotropy_supported()) {
			return;
		}
		GLfloat max_anisotropy = 1.0f;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_anisotropy);
		const GLfloat anisotropy = std::min<GLfloat>(max_anisotropy, 8.0f);
		if (anisotropy > 1.0f) {
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, anisotropy);
		}
	};

	GLuint texture = 0;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	configure_texture_antialiasing();
	const GLint internal_format = color_texture ? GL_SRGB8_ALPHA8 : GL_RGBA8;
	glTexImage2D(
		GL_TEXTURE_2D, 0, internal_format, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);
	return texture;
}

GLuint load_texture_from_file(const std::string &filename)
{
	int width = 0;
	int height = 0;
	int channels = 0;
	unsigned char *image = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
	if (image == nullptr) {
		errorout(std::string("Failed to load texture: ") + filename + " : " + stbi_failure_reason());
		return 0;
	}

	const GLuint texture = upload_rgba_texture(image, width, height, true);
	stbi_image_free(image);
	return texture;
}

PreviewMaterial generate_procedural_material(const std::string &material_name)
{
	const ParsedMaterialSpec spec = parse_material_name(material_name);
	const int texture_size = std::clamp(
		current_preview_session().render_configuration.material_texture_resolution,
		256,
		2048);
	std::vector<unsigned char> pixels(static_cast<std::size_t>(texture_size * texture_size * 4), 255);
	std::vector<unsigned char> height_pixels(static_cast<std::size_t>(texture_size * texture_size * 4), 255);
	std::vector<unsigned char> emissive_pixels(static_cast<std::size_t>(texture_size * texture_size * 4), 255);
	const MaterialSwatch primary = spec.palette.front();
	const MaterialSwatch secondary = spec.palette.size() > 1
	                                     ? spec.palette[1]
	                                     : darken_swatch(primary, 0.28f);
	const MaterialSwatch accent = spec.palette.size() > 2
	                                  ? spec.palette[2]
	                                  : lighten_swatch(primary, 0.35f);
	const float roughness = 1.0f - spec.smoothness;
	auto to_byte = [](float value) {
		return static_cast<unsigned char>(std::round(clamp01(value) * 255.0f));
	};
	auto write_swatch_pixel = [&](std::vector<unsigned char> *target,
	                              std::size_t pixel_index,
	                              const MaterialSwatch &color) {
		if (target == nullptr || pixel_index + 3 >= target->size()) {
			return;
		}
		(*target)[pixel_index + 0] = to_byte(color.r);
		(*target)[pixel_index + 1] = to_byte(color.g);
		(*target)[pixel_index + 2] = to_byte(color.b);
		(*target)[pixel_index + 3] = 255;
	};
	auto write_scalar_pixel = [&](std::vector<unsigned char> *target,
	                              std::size_t pixel_index,
	                              float value) {
		const MaterialSwatch gray = make_swatch(value, value, value);
		write_swatch_pixel(target, pixel_index, gray);
	};
	auto smooth_pulse = [](float x, float width, float feather) {
		const float inner = std::max(0.0f, width - feather);
		return glm::smoothstep(0.0f, feather, x) * (1.0f - glm::smoothstep(inner, width, x));
	};

	for (int y = 0; y < texture_size; ++y) {
		for (int x = 0; x < texture_size; ++x) {
			const float u = static_cast<float>(x) / static_cast<float>(texture_size - 1);
			const float v = static_cast<float>(y) / static_cast<float>(texture_size - 1);
			const float cloud = fractal_noise(u * 4.0f + 1.7f, v * 4.0f + 2.4f, spec.seed);
			const float detail = fractal_noise(u * 14.0f + 9.1f, v * 14.0f + 7.6f, spec.seed ^ 0x9e3779b9u, 4);
			const float cross = fractal_noise((u - v) * 8.0f + 3.2f, (u + v) * 8.0f + 4.8f, spec.seed ^ 0x7f4a7c15u, 3);
			const float line_noise = fractal_noise(u * 40.0f + 6.1f, v * 2.6f + 5.4f, spec.seed ^ 0x6c8e9cf5u, 3);
			const float micro = fractal_noise(u * 48.0f + 5.8f, v * 48.0f + 3.7f, spec.seed ^ 0x51ed270bu, 3);
			const float primary_axis = spec.vertical_pattern ? u : v;
			const float secondary_axis = spec.vertical_pattern ? v : u;

			MaterialSwatch color = mix_swatch(primary, secondary, cloud);
			MaterialSwatch emissive = make_swatch(0.0f, 0.0f, 0.0f);
			float height_value = 0.50f;
			if (spec.family == "metal") {
				const float brushed = 0.35f + 0.65f * line_noise;
				color = mix_swatch(primary, secondary, brushed);
				color = mix_swatch(color, accent, std::pow(detail, 3.0f) * 0.22f);
				color = scale_swatch(color, 0.82f + 0.24f * detail);
				height_value = 0.46f + 0.12f * brushed + 0.06f * micro;
			} else if (spec.family == "glass") {
				const float ripple = fractal_noise(u * 18.0f + 3.4f, v * 18.0f + 1.2f, spec.seed ^ 0x2ed13a49u, 4);
				color = mix_swatch(primary, secondary, 0.40f + 0.22f * cloud);
				color = lighten_swatch(color, 0.20f + 0.10f * detail);
				color = mix_swatch(color, make_swatch(0.94f, 0.97f, 1.0f), 0.18f * ripple);
				height_value = 0.49f + 0.04f * ripple;
			} else if (spec.family == "grass") {
				const float blade_field = fractal_noise(u * 22.0f + 6.0f, v * 10.0f + 1.1f, spec.seed ^ 0x3ad1d91fu, 4);
				const float blade_phase = std::fabs(std::sin((u * 36.0f + blade_field * 2.5f) * 3.1415926f));
				const float blade_mask = std::pow(blade_phase, 7.0f);
				const float tip_fade = glm::smoothstep(0.02f, 0.88f, v);
				color = mix_swatch(primary, secondary, blade_field * 0.60f + detail * 0.40f);
				color = mix_swatch(color, accent, blade_mask * 0.16f);
				color = scale_swatch(color, 0.72f + 0.34f * tip_fade);
				height_value = 0.18f + blade_mask * (0.56f + 0.18f * tip_fade) + 0.10f * micro;
			} else if (spec.family == "brick") {
				const float brick_u = u * 6.0f;
				const float brick_v = v * 10.0f;
				const float row = std::floor(brick_v);
				const float stagger = std::fmod(row, 2.0f) * 0.5f;
				const float local_u = std::fabs(std::fmod(brick_u + stagger, 1.0f));
				const float local_v = std::fabs(std::fmod(brick_v, 1.0f));
				const float mortar_x = std::min(local_u, 1.0f - local_u);
				const float mortar_y = std::min(local_v, 1.0f - local_v);
				const float mortar = std::min(mortar_x, mortar_y);
				const float brick_mask = glm::smoothstep(0.06f, 0.13f, mortar);
				const float brick_noise =
					fractal_noise((brick_u + stagger) * 1.7f, brick_v * 1.2f, spec.seed ^ 0x7ac41261u, 4);
				const MaterialSwatch mortar_color = make_swatch(0.78f, 0.75f, 0.72f);
				color = mix_swatch(primary, secondary, brick_noise);
				color = mix_swatch(color, accent, std::pow(detail, 2.0f) * 0.12f);
				color = scale_swatch(color, 0.74f + 0.22f * brick_noise);
				color = mix_swatch(mortar_color, color, brick_mask);
				height_value = lerp_scalar(0.24f + 0.04f * micro, 0.72f + 0.10f * brick_noise, brick_mask);
			} else if (spec.family == "floral") {
				const float floral_u = u * 4.0f;
				const float floral_v = v * 4.0f;
				const float local_u = std::fmod(floral_u, 1.0f) - 0.5f;
				const float local_v = std::fmod(floral_v, 1.0f) - 0.5f;
				const float angle = std::atan2(local_v, local_u);
				const float radius = std::sqrt(local_u * local_u + local_v * local_v);
				const float petal_radius = 0.16f + 0.07f * (0.5f + 0.5f * std::sin(angle * 5.0f + cloud * 2.6f));
				const float petal = 1.0f - glm::smoothstep(petal_radius, petal_radius + 0.05f, radius);
				const float core = 1.0f - glm::smoothstep(0.04f, 0.10f, radius);
				const float leaf_radius =
					std::sqrt(local_u * local_u + std::pow(local_v + 0.22f, 2.0f));
				const float leaf = 1.0f - glm::smoothstep(0.10f, 0.22f, leaf_radius);
				const MaterialSwatch background = lighten_swatch(secondary, 0.08f);
				const MaterialSwatch petals = mix_swatch(primary, accent, 0.44f + 0.20f * detail);
				const MaterialSwatch leaf_color =
					mix_swatch(accent, make_swatch(0.28f, 0.56f, 0.30f), 0.55f);
				color = mix_swatch(background, leaf_color, leaf * 0.55f);
				color = mix_swatch(color, petals, petal);
				color = mix_swatch(color, make_swatch(0.95f, 0.84f, 0.36f), core);
				height_value = 0.22f + petal * 0.42f + core * 0.22f + leaf * 0.10f;
			} else if (spec.family == "boards") {
				const float board_coord = primary_axis * 6.0f;
				const float board_index = std::floor(board_coord);
				const float board_local = std::fmod(board_coord, 1.0f);
				const float seam_distance = std::min(board_local, 1.0f - board_local);
				const float seam_mask = glm::smoothstep(0.03f, 0.10f, seam_distance);
				const float board_noise =
					fractal_noise(board_index * 0.78f + 2.4f,
					             secondary_axis * 4.0f + 3.2f,
					             spec.seed ^ 0x12a4bb91u,
					             4);
				float species_frequency = 24.0f;
				if (spec.wood_species == "walnut") {
					species_frequency = 34.0f;
				} else if (spec.wood_species == "oak") {
					species_frequency = 28.0f;
				} else if (spec.wood_species == "pine") {
					species_frequency = 18.0f;
				} else if (spec.wood_species == "cedar") {
					species_frequency = 22.0f;
				} else if (spec.wood_species == "birch") {
					species_frequency = 26.0f;
				}
				const float grain =
					0.5f + 0.5f * std::sin((secondary_axis * species_frequency + board_noise * 2.8f) * 3.1415926f);
				const float alternating = std::fmod(board_index, 2.0f);
				const MaterialSwatch plank_a =
					mix_swatch(primary, secondary, spec.two_tone ? alternating : board_noise);
				const MaterialSwatch plank_b =
					spec.two_tone
					    ? mix_swatch(secondary, accent, 0.35f)
					    : mix_swatch(primary, accent, 0.18f + 0.42f * detail);
				color = mix_swatch(plank_a, plank_b, grain * 0.58f + detail * 0.28f);
				color = scale_swatch(color, 0.74f + 0.24f * grain);
				color = mix_swatch(make_swatch(0.16f, 0.11f, 0.08f), color, seam_mask);
				height_value = lerp_scalar(0.16f + 0.03f * micro, 0.70f + 0.08f * grain, seam_mask);
			} else if (spec.family == "neon") {
				const float tile_u = u * 4.5f;
				const float tile_v = v * 4.5f;
				const float straight_band =
					smooth_pulse(std::fabs(std::fmod(tile_v, 1.0f) - 0.5f), 0.18f, 0.06f);
				const float vertical_band =
					smooth_pulse(std::fabs(std::fmod(tile_u, 1.0f) - 0.5f), 0.12f, 0.05f);
				const float diagonal_band =
					smooth_pulse(std::fabs(std::fmod((u + v) * 6.0f + cloud * 0.4f, 1.0f) - 0.5f), 0.10f, 0.04f);
				const float glow_mask = std::max(straight_band, std::max(vertical_band * 0.72f, diagonal_band * 0.92f));
				const MaterialSwatch dark_back = darken_swatch(secondary, 0.82f);
				const MaterialSwatch neon_core = lighten_swatch(accent, 0.28f);
				color = mix_swatch(dark_back, neon_core, glow_mask * 0.38f + 0.10f * micro);
				emissive = scale_swatch(neon_core, glow_mask);
				height_value = 0.38f + glow_mask * 0.28f;
			} else if (spec.family == "wood") {
				float species_frequency = 26.0f;
				float knot_frequency = 10.0f;
				if (spec.wood_species == "walnut") {
					species_frequency = 34.0f;
					knot_frequency = 8.0f;
				} else if (spec.wood_species == "oak") {
					species_frequency = 28.0f;
					knot_frequency = 11.0f;
				} else if (spec.wood_species == "pine") {
					species_frequency = 18.0f;
					knot_frequency = 7.0f;
				} else if (spec.wood_species == "cedar") {
					species_frequency = 22.0f;
					knot_frequency = 9.0f;
				} else if (spec.wood_species == "birch") {
					species_frequency = 24.0f;
					knot_frequency = 6.0f;
				}
				const float grain =
					0.5f + 0.5f * std::sin((primary_axis * species_frequency + cloud * 2.8f + line_noise * 1.2f) * 3.1415926f);
				const float secondary_grain =
					0.5f + 0.5f * std::sin((secondary_axis * species_frequency * 0.32f + detail * 3.6f) * 3.1415926f);
				const float knot =
					std::pow(fractal_noise(u * knot_frequency + 4.2f,
					                      v * knot_frequency + 2.6f,
					                      spec.seed ^ 0x5b8d0f21u,
					                      3),
					        6.0f);
				color = mix_swatch(primary, secondary, grain * 0.72f + secondary_grain * 0.28f);
				color = mix_swatch(color, accent, knot * 0.22f);
				color = scale_swatch(color, 0.72f + 0.30f * detail);
				height_value = 0.26f + 0.42f * grain + 0.14f * knot + 0.08f * micro;
			} else if (spec.family == "marble") {
				const float veins = std::pow(std::fabs(std::sin((u + v) * 18.0f + detail * 7.0f)), 3.0f);
				color = mix_swatch(primary, secondary, cloud * 0.55f + veins * 0.45f);
				color = mix_swatch(color, accent, veins * 0.20f);
				height_value = 0.44f + veins * 0.14f + 0.06f * micro;
			} else if (spec.family == "stone" || spec.family == "concrete") {
				const float speckle = std::pow(detail, 6.0f);
				color = mix_swatch(primary, secondary, cloud * 0.70f + cross * 0.30f);
				color = mix_swatch(color, accent, speckle * 0.18f);
				color = scale_swatch(color, 0.82f + 0.18f * cross);
				height_value = 0.42f + 0.20f * cross + 0.16f * detail;
			} else {
				color = mix_swatch(primary, secondary, cloud * 0.65f + detail * 0.35f);
				color = mix_swatch(color, accent, std::pow(cross, 2.0f) * 0.14f);
				height_value = 0.40f + 0.16f * cross + 0.08f * micro;
			}

			if (spec.two_tone && spec.family != "boards" && spec.family != "floral") {
				const float duo_band =
					0.5f + 0.5f * std::sin((primary_axis * 10.0f + cross * 4.0f) * 3.1415926f);
				const float duo_mix = glm::smoothstep(0.28f, 0.72f, duo_band);
				const MaterialSwatch tone_a =
					mix_swatch(primary, secondary, 0.30f + 0.18f * detail);
				const MaterialSwatch tone_b =
					mix_swatch(secondary, accent, 0.45f + 0.12f * cloud);
				color = mix_swatch(tone_a, tone_b, duo_mix * 0.82f);
				height_value = std::max(height_value, 0.34f + 0.12f * duo_mix);
			}
			if (spec.emphasize_veins) {
				const float veins =
					std::pow(std::fabs(std::sin((u * 1.8f + v * 0.6f + detail * 0.35f) * 14.0f + cloud * 4.0f)), 6.0f);
				color = mix_swatch(color, accent, veins * 0.26f);
				height_value = std::max(height_value, 0.38f + veins * 0.24f);
			}
			color = mix_swatch(color, darken_swatch(color, 0.35f), roughness * detail * 0.12f);
			if (spec.emission_strength > 0.0f && spec.family != "neon") {
				const float glow_mask =
					std::pow(std::max(detail, fractal_noise(u * 7.0f + 4.0f, v * 7.0f + 8.0f, spec.seed ^ 0x4f3a8c59u, 3)), 3.4f);
				emissive = mix_swatch(emissive, lighten_swatch(accent, 0.18f), glow_mask);
				height_value = std::max(height_value, 0.40f + glow_mask * 0.12f);
			}
			if (spec.uv_activity > 0.0f && spec.emission_strength <= 0.0f) {
				const float active_mask =
					0.5f + 0.5f * std::sin((u + v + detail) * 12.0f + cloud * 3.4f);
				emissive = mix_swatch(emissive,
				                     scale_swatch(accent, 0.16f + 0.10f * active_mask),
				                     spec.pulse_strength * 0.22f);
			}
			if (spec.family == "glass") {
				emissive = mix_swatch(emissive, make_swatch(0.02f, 0.04f, 0.05f), 0.35f * std::pow(detail, 2.5f));
			}
			if (spec.family == "metal" && spec.reflectance > 0.9f) {
				emissive = mix_swatch(emissive, make_swatch(0.03f, 0.03f, 0.03f), 0.25f * micro);
			}
			height_value = clamp01(height_value);

			const std::size_t pixel_index = static_cast<std::size_t>((y * texture_size + x) * 4);
			write_swatch_pixel(&pixels, pixel_index, color);
			write_scalar_pixel(&height_pixels, pixel_index, height_value);
			write_swatch_pixel(&emissive_pixels, pixel_index, emissive);
		}
	}

	PreviewMaterial material;
	float default_mapping_scale = 0.82f;
	if (spec.family == "grass") {
		default_mapping_scale = 1.22f;
	} else if (spec.family == "brick") {
		default_mapping_scale = 0.94f;
	} else if (spec.family == "boards") {
		default_mapping_scale = spec.vertical_pattern ? 0.70f : 0.76f;
	} else if (spec.family == "floral") {
		default_mapping_scale = 1.04f;
	} else if (spec.family == "wood") {
		default_mapping_scale = 0.72f;
	} else if (spec.family == "glass") {
		default_mapping_scale = 0.58f;
	} else if (spec.family == "metal") {
		default_mapping_scale = 0.68f;
	} else if (spec.family == "neon") {
		default_mapping_scale = 1.08f;
	} else if (spec.family == "stone" || spec.family == "concrete") {
		default_mapping_scale = 0.88f;
	}
	material.texture = upload_rgba_texture(pixels.data(), texture_size, texture_size, true);
	material.height_texture = upload_rgba_texture(height_pixels.data(), texture_size, texture_size);
	material.emissive_texture = upload_rgba_texture(emissive_pixels.data(), texture_size, texture_size, true);
	material.name = spec.name;
	material.family = spec.family;
	material.description = spec.description;
	material.opacity = spec.opacity;
	material.reflectance = spec.reflectance;
	material.smoothness = spec.smoothness;
	material.height_scale = spec.height_scale;
	material.emission_strength = spec.emission_strength;
	material.uv_rotation_speed = spec.uv_rotation_speed;
	material.uv_activity = spec.uv_activity;
	material.pulse_strength = spec.pulse_strength;
	material.normal_strength = spec.normal_strength;
	material.transmission = spec.transmission;
	material.index_of_refraction = spec.index_of_refraction;
	material.thickness = spec.thickness;
	material.attenuation_color = glm::vec3(
		spec.attenuation_color.r,
		spec.attenuation_color.g,
		spec.attenuation_color.b);
	material.texture_resolution = texture_size;

	// Set PBR material properties based on material type
	if (spec.family == "metal") {
		material.metallic = 1.0f;
		material.roughness = 1.0f - spec.smoothness;
	} else if (spec.family == "glass") {
		material.metallic = 0.0f;
		material.roughness = 0.1f;
		material.subsurface = 0.2f;
	} else if (spec.family == "wood" || spec.family == "boards") {
		material.metallic = 0.0f;
		material.roughness = 0.8f - spec.smoothness * 0.6f;
	} else if (spec.family == "stone" || spec.family == "concrete" || spec.family == "brick") {
		material.metallic = 0.0f;
		material.roughness = 0.9f;
	} else if (spec.family == "grass" || spec.family == "floral") {
		material.metallic = 0.0f;
		material.roughness = 0.7f;
		material.subsurface = 0.3f;
	} else if (spec.family == "neon") {
		material.metallic = 0.1f;
		material.roughness = 0.3f;
	} else {
		material.metallic = 0.0f;
		material.roughness = 1.0f - spec.smoothness;
	}
	material.ao = 1.0f;
	material.subsurface = std::max(0.0f, material.subsurface);
	material.anisotropic = spec.anisotropic;
	material.sheen = std::max(spec.sheen,
	                          (spec.family == "cloth" || spec.family == "floral") ? 0.3f : 0.0f);
	material.clearcoat = std::max(spec.clearcoat,
	                              (spec.family == "glass" || spec.family == "metal") ? 0.1f : 0.0f);
	material.clearcoat_roughness = spec.clearcoat > 0.0f
		? spec.clearcoat_roughness
		: (1.0f - spec.smoothness) * 0.5f;

	material.mapping_mode = static_cast<int>(PreviewMappingMode::UV);
	material.mapping_scale = default_mapping_scale;
	material.projection_axes = {
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f)};
	material.swatches = {
		glm::vec3(primary.r, primary.g, primary.b),
		glm::vec3(secondary.r, secondary.g, secondary.b),
		glm::vec3(accent.r, accent.g, accent.b)};
	// Auto-generate any missing texture maps
	auto_generate_missing_texture_maps(&material);

	return material;
}

GLuint generate_default_alpha_map(int texture_size = 64)
{
	std::vector<unsigned char> alpha_pixels(static_cast<std::size_t>(texture_size * texture_size * 4), 255);
	return upload_rgba_texture(alpha_pixels.data(), texture_size, texture_size);
}

// Generate a default glow/emissive map (mostly black with subtle variation)
GLuint generate_default_glow_map(int texture_size = 64)
{
	std::vector<unsigned char> glow_pixels(static_cast<std::size_t>(texture_size * texture_size * 4), 0);

	for (int y = 0; y < texture_size; ++y) {
		for (int x = 0; x < texture_size; ++x) {
			const float u = static_cast<float>(x) / static_cast<float>(texture_size - 1);
			const float v = static_cast<float>(y) / static_cast<float>(texture_size - 1);

			// Very subtle glow variation
			const float noise = fractal_noise(u * 8.0f + 1.0f, v * 8.0f + 2.0f, 0x12345678u, 2);
			const unsigned char glow = static_cast<unsigned char>(std::max(0.0f, noise * 10.0f));

			const std::size_t pixel_index = static_cast<std::size_t>((y * texture_size + x) * 4);
			glow_pixels[pixel_index + 0] = glow;
			glow_pixels[pixel_index + 1] = glow;
			glow_pixels[pixel_index + 2] = glow;
			glow_pixels[pixel_index + 3] = 255;
		}
	}

	return upload_rgba_texture(glow_pixels.data(), texture_size, texture_size);
}

// Generate a default normal map (mostly flat with slight variation)
GLuint generate_default_normal_map(int texture_size = 64)
{
	std::vector<unsigned char> normal_pixels(static_cast<std::size_t>(texture_size * texture_size * 4), 128);

	for (int y = 0; y < texture_size; ++y) {
		for (int x = 0; x < texture_size; ++x) {
			const float u = static_cast<float>(x) / static_cast<float>(texture_size - 1);
			const float v = static_cast<float>(y) / static_cast<float>(texture_size - 1);

			// Generate slight normal variation
			const float noise_x = fractal_noise(u * 12.0f + 3.0f, v * 12.0f + 4.0f, 0xABCDEF01u, 3) - 0.5f;
			const float noise_y = fractal_noise(u * 12.0f + 5.0f, v * 12.0f + 6.0f, 0xFEDCBA12u, 3) - 0.5f;

			// Convert to normal map colors (default flat is 128, 128, 255)
			const unsigned char normal_x = static_cast<unsigned char>(std::clamp(128 + noise_x * 30.0f, 0.0f, 255.0f));
			const unsigned char normal_y = static_cast<unsigned char>(std::clamp(128 + noise_y * 30.0f, 0.0f, 255.0f));

			const std::size_t pixel_index = static_cast<std::size_t>((y * texture_size + x) * 4);
			normal_pixels[pixel_index + 0] = normal_x;
			normal_pixels[pixel_index + 1] = normal_y;
			normal_pixels[pixel_index + 2] = 255;  // Z is always up
			normal_pixels[pixel_index + 3] = 255;
		}
	}

	return upload_rgba_texture(normal_pixels.data(), texture_size, texture_size);
}

GLuint generate_default_roughness_map(float smoothness, int texture_size = 64)
{
	std::vector<unsigned char> roughness_pixels(static_cast<std::size_t>(texture_size * texture_size * 4), 128);
	(void)smoothness;

	for (int y = 0; y < texture_size; ++y) {
		for (int x = 0; x < texture_size; ++x) {
			const float u = static_cast<float>(x) / static_cast<float>(texture_size - 1);
			const float v = static_cast<float>(y) / static_cast<float>(texture_size - 1);

			// Add slight variation
			const float noise = fractal_noise(u * 10.0f + 7.0f, v * 10.0f + 8.0f, 0x23456789u, 2);
			const unsigned char roughness = static_cast<unsigned char>(std::clamp(
				228.0f + noise * 27.0f, 0.0f, 255.0f));

			const std::size_t pixel_index = static_cast<std::size_t>((y * texture_size + x) * 4);
			roughness_pixels[pixel_index + 0] = roughness;
			roughness_pixels[pixel_index + 1] = roughness;
			roughness_pixels[pixel_index + 2] = roughness;
			roughness_pixels[pixel_index + 3] = 255;
		}
	}

	return upload_rgba_texture(roughness_pixels.data(), texture_size, texture_size);
}

GLuint generate_default_metallic_map(int texture_size = 64)
{
	std::vector<unsigned char> metallic_pixels(static_cast<std::size_t>(texture_size * texture_size * 4), 255);

	for (int y = 0; y < texture_size; ++y) {
		for (int x = 0; x < texture_size; ++x) {
			const unsigned char metallic = 255;

			const std::size_t pixel_index = static_cast<std::size_t>((y * texture_size + x) * 4);
			metallic_pixels[pixel_index + 0] = metallic;
			metallic_pixels[pixel_index + 1] = metallic;
			metallic_pixels[pixel_index + 2] = metallic;
			metallic_pixels[pixel_index + 3] = 255;
		}
	}

	return upload_rgba_texture(metallic_pixels.data(), texture_size, texture_size);
}

// Generate a default ambient occlusion map (mostly white with some darkening)
GLuint generate_default_ao_map(int texture_size = 64)
{
	std::vector<unsigned char> ao_pixels(static_cast<std::size_t>(texture_size * texture_size * 4), 255);

	for (int y = 0; y < texture_size; ++y) {
		for (int x = 0; x < texture_size; ++x) {
			const float u = static_cast<float>(x) / static_cast<float>(texture_size - 1);
			const float v = static_cast<float>(y) / static_cast<float>(texture_size - 1);

			// Generate some subtle AO patterns
			const float noise = fractal_noise(u * 6.0f + 11.0f, v * 6.0f + 12.0f, 0x456789ABu, 2);
			const float ao = 1.0f - (noise * 0.15f);  // Slight darkening

			const unsigned char ao_value = static_cast<unsigned char>(std::clamp(ao * 255.0f, 0.0f, 255.0f));

			const std::size_t pixel_index = static_cast<std::size_t>((y * texture_size + x) * 4);
			ao_pixels[pixel_index + 0] = ao_value;
			ao_pixels[pixel_index + 1] = ao_value;
			ao_pixels[pixel_index + 2] = ao_value;
			ao_pixels[pixel_index + 3] = 255;
		}
	}

	return upload_rgba_texture(ao_pixels.data(), texture_size, texture_size);
}

// Auto-generate all missing texture maps for a material
void auto_generate_missing_texture_maps(PreviewMaterial *material)
{
	if (material == nullptr) {
		return;
	}
	const int auxiliary_texture_resolution =
		std::clamp(material->texture_resolution, 64, 512);

	// Generate alpha map if missing
	if (material->alpha_texture == 0) {
		material->alpha_texture = generate_default_alpha_map(auxiliary_texture_resolution);
	}

	// Generate glow map if missing
	if (material->glow_texture == 0) {
		material->glow_texture = generate_default_glow_map(auxiliary_texture_resolution);
	}

	// Generate normal map if missing
	if (material->normal_texture == 0) {
		material->normal_texture = generate_default_normal_map(auxiliary_texture_resolution);
	}

	// Generate roughness map if missing
	if (material->roughness_texture == 0) {
		material->roughness_texture = generate_default_roughness_map(
			material->smoothness, auxiliary_texture_resolution);
	}

	// Generate metallic map if missing
	if (material->metallic_texture == 0) {
		material->metallic_texture = generate_default_metallic_map(auxiliary_texture_resolution);
	}

	// Generate ambient occlusion map if missing
	if (material->ao_texture == 0) {
		material->ao_texture = generate_default_ao_map(auxiliary_texture_resolution);
	}
}

bool is_user_texture_slot_name(std::string_view material_name)
{
	constexpr std::string_view kPrefix = "usertexture";
	if (material_name.size() <= kPrefix.size() || material_name.rfind(kPrefix, 0) != 0) {
		return false;
	}

	for (char ch : material_name.substr(kPrefix.size())) {
		if (std::isdigit(static_cast<unsigned char>(ch)) == 0) {
			return false;
		}
	}
	return true;
}

BackendTextureSlot make_texture_slot_placeholder(int slot_index)
{
	BackendTextureSlot texture;
	texture.slot = "usertexture" + std::to_string(slot_index);
	texture.display_name = texture.slot;
	return texture;
}

const BackendTextureSlot *find_texture_library_entry(const TextureLibraryState &state,
                                                     const std::string &slot)
{
	for (const BackendTextureSlot &entry : state.entries) {
		if (entry.slot == slot) {
			return &entry;
		}
	}
	return nullptr;
}

bool decode_texture_image_bytes(const std::string &bytes,
                                std::vector<unsigned char> *rgba_pixels,
                                int *width,
                                int *height)
{
	if (rgba_pixels == nullptr || width == nullptr || height == nullptr || bytes.empty()) {
		return false;
	}

	int channels = 0;
	unsigned char *decoded = stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(bytes.data()),
	                                               static_cast<int>(bytes.size()),
	                                               width,
	                                               height,
	                                               &channels,
	                                               STBI_rgb_alpha);
	if (decoded == nullptr) {
		return false;
	}

	rgba_pixels->assign(decoded, decoded + static_cast<std::size_t>((*width) * (*height) * 4));
	stbi_image_free(decoded);
	return true;
}

PreviewMaterial generate_missing_user_texture_material(const std::string &slot,
                                                       const std::string &message)
{
	PreviewMaterial material = generate_procedural_material("softwhitematteplaster");
	material.name = slot;
	material.family = "user texture";
	material.description = message;
	material.mapping_scale = 1.0f;
	material.height_scale = 0.0f;
	material.emission_strength = 0.0f;

	// Set default PBR material properties for missing texture fallback
	material.metallic = 0.0f;
	material.roughness = 0.8f;
	material.ao = 1.0f;
	material.subsurface = 0.0f;
	material.anisotropic = 0.0f;
	material.sheen = 0.0f;
	material.clearcoat = 0.0f;
	material.clearcoat_roughness = 0.0f;

	// Auto-generate any missing texture maps
	auto_generate_missing_texture_maps(&material);

	return material;
}

PreviewMaterial generate_backend_texture_material(EditorWorkspaceSession *app_state, const std::string &slot)
{
	if (app_state == nullptr) {
		return generate_missing_user_texture_material(slot, "Texture library is not available.");
	}

	const BackendTextureSlot *entry = find_texture_library_entry(app_state->texture_library, slot);
	if (entry == nullptr || !entry->active) {
		return generate_missing_user_texture_material(slot, "No active backend texture is stored in this slot.");
	}
	if (!app_state->authentication.session.authenticated || trim_copy(app_state->authentication.config.backend_base_url).empty()) {
		return generate_missing_user_texture_material(slot, "Sign in to load backend textures.");
	}

	std::string image_bytes;
	std::string error;
	BackendTextureLibraryRepository texture_repository(app_state->authentication.config,
	                                                   app_state->authentication.session);
	if (!texture_repository.fetchPreviewImage(slot, &image_bytes, &error)) {
		if (!error.empty()) {
			debugout("Texture load failed for " + slot + ": " + error);
		}
		return generate_missing_user_texture_material(slot, "Could not download the backend texture.");
	}

	int width = 0;
	int height = 0;
	std::vector<unsigned char> rgba_pixels;
	if (!decode_texture_image_bytes(image_bytes, &rgba_pixels, &width, &height) ||
	    rgba_pixels.empty() ||
	    width <= 0 ||
	    height <= 0) {
		debugout("Texture decode failed for " + slot + ".");
		return generate_missing_user_texture_material(slot, "Could not decode the backend texture image.");
	}

	PreviewMaterial material;
	material.texture = upload_rgba_texture(rgba_pixels.data(), width, height, true);
	const unsigned char flat_height[] = {128, 128, 128, 255};
	const unsigned char black_emissive[] = {0, 0, 0, 255};
	material.height_texture = upload_rgba_texture(flat_height, 1, 1);
	material.emissive_texture = upload_rgba_texture(black_emissive, 1, 1);
	if (material.texture == 0 || material.height_texture == 0 || material.emissive_texture == 0) {
		destroy_preview_material_textures(&material);
		return generate_missing_user_texture_material(slot, "OpenGL texture upload failed for the backend texture.");
	}

	double total_weight = 0.0;
	glm::dvec3 accumulated(0.0);
	const std::size_t pixel_count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
	const std::size_t sample_step = std::max<std::size_t>(1, pixel_count / 2048);
	for (std::size_t pixel_index = 0; pixel_index < pixel_count; pixel_index += sample_step) {
		const std::size_t base = pixel_index * 4;
		const double alpha_weight = static_cast<double>(rgba_pixels[base + 3]) / 255.0;
		total_weight += alpha_weight;
		accumulated.r += static_cast<double>(rgba_pixels[base + 0]) / 255.0 * alpha_weight;
		accumulated.g += static_cast<double>(rgba_pixels[base + 1]) / 255.0 * alpha_weight;
		accumulated.b += static_cast<double>(rgba_pixels[base + 2]) / 255.0 * alpha_weight;
	}
	glm::vec3 average_color = total_weight > 0.0
	                              ? glm::vec3(accumulated / total_weight)
	                              : glm::vec3(0.75f, 0.75f, 0.75f);
	const glm::vec3 dark_color = glm::clamp(average_color * 0.72f, glm::vec3(0.0f), glm::vec3(1.0f));
	const glm::vec3 light_color = glm::clamp(average_color * 0.45f + glm::vec3(0.55f), glm::vec3(0.0f), glm::vec3(1.0f));

	material.name = slot;
	material.family = "user texture";
	std::ostringstream description;
	description << entry->display_name;
	if (!entry->source.empty()) {
		description << " • source " << entry->source;
	}
	if (!entry->updated_at.empty()) {
		description << " • updated " << entry->updated_at;
	}
	if (!entry->prompt.empty()) {
		description << " • " << entry->prompt;
	}
	material.description = description.str();
	material.opacity = std::clamp(entry->alpha, 0.0f, 1.0f);
	material.reflectance = 0.24f;
	material.smoothness = 0.52f;
	material.height_scale = 0.0f;
	material.emission_strength = 0.0f;
	material.uv_rotation_speed = 0.0f;
	material.uv_activity = 0.0f;
	material.pulse_strength = 0.0f;
	material.texture_resolution = std::max(width, height);

	// Set default PBR material properties for user-uploaded textures
	material.metallic = 0.0f;
	material.roughness = 0.5f;
	material.ao = 1.0f;
	material.subsurface = 0.0f;
	material.anisotropic = 0.0f;
	material.sheen = 0.0f;
	material.clearcoat = 0.0f;
	material.clearcoat_roughness = 0.0f;

	material.mapping_mode = static_cast<int>(PreviewMappingMode::UV);
	material.mapping_scale = 1.0f;
	material.projection_axes = {
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f)};
	material.swatches = {average_color, dark_color, light_color};
	// Auto-generate any missing texture maps
	auto_generate_missing_texture_maps(&material);

	return material;
}

void destroy_preview_material_textures(PreviewMaterial *material)
{
	if (material == nullptr) {
		return;
	}
	if (material->texture != 0) {
		glDeleteTextures(1, &material->texture);
		material->texture = 0;
	}
	if (material->height_texture != 0) {
		glDeleteTextures(1, &material->height_texture);
		material->height_texture = 0;
	}
	if (material->emissive_texture != 0) {
		glDeleteTextures(1, &material->emissive_texture);
		material->emissive_texture = 0;
	}
	if (material->alpha_texture != 0) {
		glDeleteTextures(1, &material->alpha_texture);
		material->alpha_texture = 0;
	}
	if (material->glow_texture != 0) {
		glDeleteTextures(1, &material->glow_texture);
		material->glow_texture = 0;
	}
	if (material->normal_texture != 0) {
		glDeleteTextures(1, &material->normal_texture);
		material->normal_texture = 0;
	}
	if (material->roughness_texture != 0) {
		glDeleteTextures(1, &material->roughness_texture);
		material->roughness_texture = 0;
	}
	if (material->metallic_texture != 0) {
		glDeleteTextures(1, &material->metallic_texture);
		material->metallic_texture = 0;
	}
	if (material->ao_texture != 0) {
		glDeleteTextures(1, &material->ao_texture);
		material->ao_texture = 0;
	}
}

void sync_active_materials(EditorWorkspaceSession *app_state, const std::vector<std::string> &material_names)
{
	active_materials.clear();
	active_materials.reserve(material_names.size());
	std::unordered_set<std::string> active_names(material_names.begin(), material_names.end());
	for (auto it = material_cache.begin(); it != material_cache.end();) {
		if (active_names.find(it->first) == active_names.end()) {
			destroy_preview_material_textures(&it->second);
			it = material_cache.erase(it);
			continue;
		}
		++it;
	}
	for (const std::string &material_name : material_names) {
		auto it = material_cache.find(material_name);
		if (it == material_cache.end()) {
			PreviewMaterial generated_material = is_user_texture_slot_name(material_name)
			                                        ? generate_backend_texture_material(app_state, material_name)
			                                        : generate_procedural_material(material_name);
			it = material_cache.emplace(material_name, std::move(generated_material)).first;
		}
		active_materials.push_back(it->second);
	}
}

void destroy_materials()
{
	for (auto &entry : material_cache) {
		destroy_preview_material_textures(&entry.second);
	}
	material_cache.clear();
	active_materials.clear();
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

std::string display_document_name(const EditorWorkspaceSession *app_state)
{
	std::string label;
	if (app_state != nullptr && !app_state->storage_identity.cloud_title.empty()) {
		label = app_state->storage_identity.cloud_title;
	} else if (app_state != nullptr && !app_state->document.local_path.empty()) {
		label = app_state->document.local_path.filename().string();
	} else {
		label = "Untitled";
	}
	if (app_state != nullptr && app_state->document.dirty) {
		label += " *";
	}
	return label;
}

std::string lowercase_copy(std::string value)
{
	for (char &character : value) {
		character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
	}
	return value;
}

std::string build_default_new_document_text_internal()
{
	return "X -> R Height ( 4 8 ) Y\n"
	       "Y -> S ( 1 Height 1 ) I ( Cube softblueglossysmoothceramic 1 1 )\n";
}

void join_regeneration_worker_if_idle()
{
	if (scene_regeneration_coordinator != nullptr) {
		scene_regeneration_coordinator->joinWorkerIfIdle();
	}
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

enum class ConnectionSemanticKind {
	StaticPositional,
	MotionRotational,
	MotionLinear,
	ElectricalPower,
	ElectricalData,
	TemperatureHeating,
	TemperatureCooling
};

enum class ConnectionFlowRole {
	Static,
	Input,
	Output,
	Bidirectional
};

bool contains_any_token(const std::string &text, std::initializer_list<const char *> needles)
{
	for (const char *needle : needles) {
		if (needle != nullptr && text.find(needle) != std::string::npos) {
			return true;
		}
	}
	return false;
}

std::string connection_semantic_text(const StlCatalogEntry &entry,
                                     const StlCatalogEntry::ConnectionPoint *point,
                                     const StlCatalogEntry::ConnectionSurface *surface,
                                     const StlCatalogEntry::DetectedConnection *feature)
{
	std::string text = lowercase_copy(entry.category + " " + entry.name + " " + entry.description + " " +
	                                  entry.short_description + " ");
	if (point != nullptr) {
		text += lowercase_copy(point->name + " " + point->type + " " + point->movement_type + " " + point->notes + " ");
	}
	if (surface != nullptr) {
		text += lowercase_copy(surface->name + " " + surface->shape + " " + surface->plane + " " +
		                       surface->thread_type + " " + surface->notes + " ");
	}
	if (feature != nullptr) {
		text += lowercase_copy(feature->name + " " + feature->primitive + " " + feature->feature_role + " " +
		                       feature->plane + " " + feature->metric_size + " " + feature->thread_type + " " +
		                       feature->source_module + " " + feature->evidence + " ");
	}
	return text;
}

const StlCatalogEntry::ConnectionPoint *find_connection_point(const StlCatalogEntry &entry, const std::string &name)
{
	for (const StlCatalogEntry::ConnectionPoint &point : entry.connection_points) {
		if (point.name == name) {
			return &point;
		}
	}
	return nullptr;
}

const StlCatalogEntry::ConnectionSurface *find_connection_surface(const StlCatalogEntry &entry,
                                                                  const std::string &name)
{
	for (const StlCatalogEntry::ConnectionSurface &surface : entry.connection_surfaces) {
		if (surface.source_connection_point == name) {
			return &surface;
		}
	}
	return nullptr;
}

ConnectionSemanticKind classify_connection_kind(const StlCatalogEntry &entry,
                                                const StlCatalogEntry::ConnectionPoint *point,
                                                const StlCatalogEntry::ConnectionSurface *surface,
                                                const StlCatalogEntry::DetectedConnection *feature)
{
	if (point != nullptr) {
		if (point->movement_type == "rotational") {
			return ConnectionSemanticKind::MotionRotational;
		}
		if (point->movement_type == "linear") {
			return ConnectionSemanticKind::MotionLinear;
		}
	}

	const std::string text = connection_semantic_text(entry, point, surface, feature);
	if (contains_any_token(text, {"printer_hotend", "hotend", "heater", "heated", "nozzle"}) &&
	    !contains_any_token(text, {"mount_pattern", "fastener_pattern", "mount face"})) {
		return ConnectionSemanticKind::TemperatureHeating;
	}
	if (contains_any_token(text, {"fan_aperture", "airflow", "printer_fan", "cooling", "cool"})) {
		return ConnectionSemanticKind::TemperatureCooling;
	}
	if (contains_any_token(text, {"power_jack", "barrel_jack", "socket_box", "fuseholder", "mains", "terminal", "spade", "ssr"})) {
		return ConnectionSemanticKind::ElectricalPower;
	}
	if (contains_any_token(text, {"usb", "rj45", "ribbon", "camera", "sensor", "pcb", "board_capture", "pcb_outline", "connector", "optical"})) {
		return ConnectionSemanticKind::ElectricalData;
	}
	if (contains_any_token(text, {"motor_shaft", "motor_output", "hinge", "pivot", "gear_bore", "gear_mesh", "shaft_bore", "threaded_axis", "captured_fastener", "bearing_support"})) {
		return ConnectionSemanticKind::MotionRotational;
	}
	if (contains_any_token(text, {"linear_bearing", "linear_guide", "linear_rail", "carriage_track", "supported_linear_rail", "guide_axis", "rod_axis", "shaft_support", "filament_path", "rack_track"})) {
		return ConnectionSemanticKind::MotionLinear;
	}
	return ConnectionSemanticKind::StaticPositional;
}

ConnectionFlowRole classify_connection_flow(const StlCatalogEntry &entry,
                                            const StlCatalogEntry::ConnectionPoint *point,
                                            const StlCatalogEntry::ConnectionSurface *surface,
                                            const StlCatalogEntry::DetectedConnection *feature,
                                            ConnectionSemanticKind kind)
{
	if (kind == ConnectionSemanticKind::StaticPositional) {
		return ConnectionFlowRole::Static;
	}

	const std::string text = connection_semantic_text(entry, point, surface, feature);
	if (contains_any_token(text, {"output", "airflow", "lens_axis", "optical_axis", "drive_axis"})) {
		return ConnectionFlowRole::Output;
	}
	if (contains_any_token(text, {"socket", "jack", "capture", "seat", "mounting_groove", "panel_interface"})) {
		return ConnectionFlowRole::Input;
	}
	if (feature != nullptr) {
		if (feature->feature_role == "hole" || feature->feature_role == "threaded_hole") {
			return ConnectionFlowRole::Input;
		}
		if (feature->feature_role == "protrusion" || feature->feature_role == "thread") {
			return ConnectionFlowRole::Output;
		}
		if (feature->feature_role == "tube") {
			return ConnectionFlowRole::Bidirectional;
		}
	}
	if (kind == ConnectionSemanticKind::TemperatureHeating ||
	    kind == ConnectionSemanticKind::TemperatureCooling) {
		return ConnectionFlowRole::Output;
	}
	return ConnectionFlowRole::Bidirectional;
}

ImVec4 color_for_connection_kind(ConnectionSemanticKind kind, float alpha = 1.0f)
{
	switch (kind) {
	case ConnectionSemanticKind::MotionRotational:
		return color_from_hex(0xff4fd8, alpha);
	case ConnectionSemanticKind::MotionLinear:
		return color_from_hex(0x6bff7a, alpha);
	case ConnectionSemanticKind::ElectricalPower:
		return color_from_hex(0xfff63b, alpha);
	case ConnectionSemanticKind::ElectricalData:
		return color_from_hex(0x32fff3, alpha);
	case ConnectionSemanticKind::TemperatureHeating:
		return color_from_hex(0xff7a1a, alpha);
	case ConnectionSemanticKind::TemperatureCooling:
		return color_from_hex(0x4db8ff, alpha);
	case ConnectionSemanticKind::StaticPositional:
	default:
		return color_from_hex(0xb97cff, alpha);
	}
}

ImVec4 color_for_flow_role(ConnectionFlowRole role, float alpha = 1.0f)
{
	switch (role) {
	case ConnectionFlowRole::Input:
		return color_from_hex(0xf8f7ff, alpha);
	case ConnectionFlowRole::Output:
		return color_from_hex(0xff4fd8, alpha);
	case ConnectionFlowRole::Bidirectional:
		return color_from_hex(0xfff8a6, alpha);
	case ConnectionFlowRole::Static:
	default:
		return color_from_hex(0xcbb0ff, alpha);
	}
}

ImVec4 color_for_plane(const std::string &plane, float alpha = 1.0f)
{
	if (plane == "xy") {
		return color_from_hex(0x22f0ff, alpha);
	}
	if (plane == "xz") {
		return color_from_hex(0x7cff3b, alpha);
	}
	if (plane == "yz") {
		return color_from_hex(0xffc531, alpha);
	}
	return color_from_hex(0xffffff, alpha);
}

const char *label_for_connection_kind(ConnectionSemanticKind kind)
{
	switch (kind) {
	case ConnectionSemanticKind::MotionRotational:
		return "motion.rotational";
	case ConnectionSemanticKind::MotionLinear:
		return "motion.linear";
	case ConnectionSemanticKind::ElectricalPower:
		return "electrical.power";
	case ConnectionSemanticKind::ElectricalData:
		return "electrical.data";
	case ConnectionSemanticKind::TemperatureHeating:
		return "temperature.heating";
	case ConnectionSemanticKind::TemperatureCooling:
		return "temperature.cooling";
	case ConnectionSemanticKind::StaticPositional:
	default:
		return "static.positional";
	}
}

const char *label_for_flow_role(ConnectionFlowRole role)
{
	switch (role) {
	case ConnectionFlowRole::Input:
		return "input";
	case ConnectionFlowRole::Output:
		return "output";
	case ConnectionFlowRole::Bidirectional:
		return "bidirectional";
	case ConnectionFlowRole::Static:
	default:
		return "static";
	}
}

glm::vec3 to_vec3(const std::array<float, 3> &value)
{
	return glm::vec3(value[0], value[1], value[2]);
}

glm::vec3 to_vec3(const std::array<int, 3> &value)
{
	return glm::vec3(static_cast<float>(value[0]),
	                 static_cast<float>(value[1]),
	                 static_cast<float>(value[2]));
}

glm::vec3 safe_normalize(const glm::vec3 &value, const glm::vec3 &fallback)
{
	const float length = glm::length(value);
	if (length <= 0.00001f) {
		return fallback;
	}
	return value / length;
}

glm::vec3 transform_overlay_position(const PrimitiveInstance &instance, const glm::vec3 &local_position)
{
	return glm::vec3(instance.primary_transform * glm::vec4(local_position, 1.0f));
}

glm::vec3 transform_overlay_direction(const PrimitiveInstance &instance, const glm::vec3 &local_direction)
{
	const glm::vec3 transformed = glm::mat3(instance.primary_transform) * local_direction;
	return safe_normalize(transformed, glm::vec3(0.0f, 1.0f, 0.0f));
}

struct LocalMeshBounds {
	glm::vec3 min{0.0f};
	glm::vec3 max{0.0f};
	glm::vec3 size{1.0f};
	float max_extent = 1.0f;
	bool valid = false;
};

LocalMeshBounds compute_local_mesh_bounds(const Mesh &mesh)
{
	LocalMeshBounds bounds;
	if (mesh.vertices.empty()) {
		return bounds;
	}

	bounds.min = mesh.vertices.front();
	bounds.max = mesh.vertices.front();
	for (const glm::vec3 &vertex : mesh.vertices) {
		bounds.min = glm::min(bounds.min, vertex);
		bounds.max = glm::max(bounds.max, vertex);
	}
	bounds.size = bounds.max - bounds.min;
	bounds.max_extent = std::max(std::max(bounds.size.x, bounds.size.y), std::max(bounds.size.z, 0.001f));
	bounds.valid = true;
	return bounds;
}

bool has_nonzero_size(const std::array<float, 3> &size)
{
	return size[0] > 0.0001f || size[1] > 0.0001f || size[2] > 0.0001f;
}

float max_size_component(const std::array<float, 3> &size)
{
	return std::max(size[0], std::max(size[1], size[2]));
}

LocalMeshBounds local_bounds_for_entry(const StlCatalogEntry &entry, const Mesh &mesh)
{
	LocalMeshBounds bounds = compute_local_mesh_bounds(mesh);
	const auto &bbox = entry.stl_geometry.bounding_box;
	if (!has_nonzero_size(bbox.size)) {
		return bounds;
	}

	bounds.min = to_vec3(bbox.min);
	bounds.max = to_vec3(bbox.max);
	bounds.size = to_vec3(bbox.size);
	bounds.max_extent = std::max(std::max(bounds.size.x, bounds.size.y), std::max(bounds.size.z, 0.001f));
	bounds.valid = true;
	return bounds;
}

int axis_index_from_vector(const std::array<int, 3> &axis_vector)
{
	int index = 2;
	int magnitude = 0;
	for (int current = 0; current < 3; ++current) {
		const int value = std::abs(axis_vector[current]);
		if (value > magnitude) {
			magnitude = value;
			index = current;
		}
	}
	return index;
}

bool is_cardinal_axis_vector(const std::array<int, 3> &axis_vector)
{
	int nonzero_components = 0;
	for (int value : axis_vector) {
		if (std::abs(value) > 1) {
			return false;
		}
		if (value != 0) {
			++nonzero_components;
		}
	}
	return nonzero_components == 1;
}

std::string plane_from_axis_vector(const std::array<int, 3> &axis_vector)
{
	const int axis_index = axis_index_from_vector(axis_vector);
	if (axis_index == 0) {
		return "yz";
	}
	if (axis_index == 1) {
		return "xz";
	}
	return "xy";
}

glm::vec3 local_axis_direction(const std::array<int, 3> &axis_vector)
{
	const int axis_index = axis_index_from_vector(axis_vector);
	const int sign = axis_vector[axis_index] < 0 ? -1 : 1;
	if (axis_index == 0) {
		return glm::vec3(static_cast<float>(sign), 0.0f, 0.0f);
	}
	if (axis_index == 1) {
		return glm::vec3(0.0f, static_cast<float>(sign), 0.0f);
	}
	return glm::vec3(0.0f, 0.0f, static_cast<float>(sign));
}

void local_basis_from_axis(const std::array<int, 3> &axis_vector,
                           glm::vec3 *axis_local,
                           glm::vec3 *u_local,
                           glm::vec3 *v_local)
{
	const int axis_index = axis_index_from_vector(axis_vector);
	const int sign = axis_vector[axis_index] < 0 ? -1 : 1;
	if (axis_index == 0) {
		*axis_local = glm::vec3(static_cast<float>(sign), 0.0f, 0.0f);
		*u_local = glm::vec3(0.0f, 1.0f, 0.0f);
		*v_local = glm::vec3(0.0f, 0.0f, 1.0f);
		return;
	}
	if (axis_index == 1) {
		*axis_local = glm::vec3(0.0f, static_cast<float>(sign), 0.0f);
		*u_local = glm::vec3(1.0f, 0.0f, 0.0f);
		*v_local = glm::vec3(0.0f, 0.0f, 1.0f);
		return;
	}
	*axis_local = glm::vec3(0.0f, 0.0f, static_cast<float>(sign));
	*u_local = glm::vec3(1.0f, 0.0f, 0.0f);
	*v_local = glm::vec3(0.0f, 1.0f, 0.0f);
}

void local_basis_from_plane(const std::string &plane,
                            glm::vec3 *u_local,
                            glm::vec3 *v_local,
                            glm::vec3 *normal_local)
{
	if (plane == "xz") {
		*u_local = glm::vec3(1.0f, 0.0f, 0.0f);
		*v_local = glm::vec3(0.0f, 0.0f, 1.0f);
		*normal_local = glm::vec3(0.0f, 1.0f, 0.0f);
		return;
	}
	if (plane == "yz") {
		*u_local = glm::vec3(0.0f, 1.0f, 0.0f);
		*v_local = glm::vec3(0.0f, 0.0f, 1.0f);
		*normal_local = glm::vec3(1.0f, 0.0f, 0.0f);
		return;
	}
	*u_local = glm::vec3(1.0f, 0.0f, 0.0f);
	*v_local = glm::vec3(0.0f, 1.0f, 0.0f);
	*normal_local = glm::vec3(0.0f, 0.0f, 1.0f);
}

void local_basis_from_surface_axis(const std::array<int, 3> &axis_vector,
                                   const std::string &fallback_plane,
                                   glm::vec3 *u_local,
                                   glm::vec3 *v_local,
                                   glm::vec3 *normal_local)
{
	if (is_cardinal_axis_vector(axis_vector)) {
		local_basis_from_axis(axis_vector, normal_local, u_local, v_local);
		return;
	}
	local_basis_from_plane(fallback_plane, u_local, v_local, normal_local);
}

float axial_extent_from_size(const std::array<int, 3> &axis_vector,
                             const std::array<float, 3> &size)
{
	return std::max(0.0f, size[axis_index_from_vector(axis_vector)]);
}

std::pair<float, float> radial_extents_from_size(const std::array<int, 3> &axis_vector,
                                                 const std::array<float, 3> &size)
{
	const int axis_index = axis_index_from_vector(axis_vector);
	if (axis_index == 0) {
		return {std::max(0.0f, size[1]), std::max(0.0f, size[2])};
	}
	if (axis_index == 1) {
		return {std::max(0.0f, size[0]), std::max(0.0f, size[2])};
	}
	return {std::max(0.0f, size[0]), std::max(0.0f, size[1])};
}

std::pair<float, float> plane_extents_from_size(const std::string &plane,
                                                const std::array<float, 3> &size)
{
	if (plane == "xz") {
		return {std::max(0.0f, size[0]), std::max(0.0f, size[2])};
	}
	if (plane == "yz") {
		return {std::max(0.0f, size[1]), std::max(0.0f, size[2])};
	}
	return {std::max(0.0f, size[0]), std::max(0.0f, size[1])};
}

void append_overlay_segment(std::vector<PreviewOverlayVertex> *vertices,
                            const glm::vec3 &a,
                            const glm::vec3 &b,
                            const ImVec4 &color)
{
	if (vertices == nullptr) {
		return;
	}
	vertices->push_back({a.x, a.y, a.z, color.x, color.y, color.z, color.w});
	vertices->push_back({b.x, b.y, b.z, color.x, color.y, color.z, color.w});
}

void append_overlay_arrowhead(std::vector<PreviewOverlayVertex> *vertices,
                              const glm::vec3 &tip,
                              const glm::vec3 &direction,
                              const glm::vec3 &reference_up,
                              float size,
                              const ImVec4 &color)
{
	const glm::vec3 forward = safe_normalize(direction, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 right = glm::cross(forward, reference_up);
	if (glm::length(right) <= 0.00001f) {
		right = glm::cross(forward, glm::vec3(1.0f, 0.0f, 0.0f));
	}
	right = safe_normalize(right, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::vec3 up = safe_normalize(glm::cross(right, forward), reference_up);
	const glm::vec3 base = tip - forward * size;
	append_overlay_segment(vertices, tip, base + right * (size * 0.45f), color);
	append_overlay_segment(vertices, tip, base - right * (size * 0.45f), color);
	append_overlay_segment(vertices, tip, base + up * (size * 0.45f), color);
	append_overlay_segment(vertices, tip, base - up * (size * 0.45f), color);
}

void append_overlay_ring(std::vector<PreviewOverlayVertex> *vertices,
                         const PrimitiveInstance &instance,
                         const glm::vec3 &center_local,
                         const glm::vec3 &u_local,
                         const glm::vec3 &v_local,
                         float radius_u,
                         float radius_v,
                         const ImVec4 &color,
                         int segments = 24)
{
	if (vertices == nullptr || radius_u <= 0.0001f || radius_v <= 0.0001f || segments < 3) {
		return;
	}

	const float step = 6.28318530718f / static_cast<float>(segments);
	for (int segment = 0; segment < segments; ++segment) {
		const float angle_a = step * static_cast<float>(segment);
		const float angle_b = step * static_cast<float>(segment + 1);
		const glm::vec3 local_a =
			center_local + u_local * (std::cos(angle_a) * radius_u) + v_local * (std::sin(angle_a) * radius_v);
		const glm::vec3 local_b =
			center_local + u_local * (std::cos(angle_b) * radius_u) + v_local * (std::sin(angle_b) * radius_v);
		append_overlay_segment(vertices,
		                      transform_overlay_position(instance, local_a),
		                      transform_overlay_position(instance, local_b),
		                      color);
	}
}

void append_overlay_plane_frame(std::vector<PreviewOverlayVertex> *vertices,
                                const PrimitiveInstance &instance,
                                const glm::vec3 &center_local,
                                const glm::vec3 &u_local,
                                const glm::vec3 &v_local,
                                float half_u,
                                float half_v,
                                const ImVec4 &frame_color,
                                const ImVec4 &cross_color)
{
	if (vertices == nullptr || half_u <= 0.0001f || half_v <= 0.0001f) {
		return;
	}

	const glm::vec3 c0 = transform_overlay_position(instance, center_local - u_local * half_u - v_local * half_v);
	const glm::vec3 c1 = transform_overlay_position(instance, center_local + u_local * half_u - v_local * half_v);
	const glm::vec3 c2 = transform_overlay_position(instance, center_local + u_local * half_u + v_local * half_v);
	const glm::vec3 c3 = transform_overlay_position(instance, center_local - u_local * half_u + v_local * half_v);
	append_overlay_segment(vertices, c0, c1, frame_color);
	append_overlay_segment(vertices, c1, c2, frame_color);
	append_overlay_segment(vertices, c2, c3, frame_color);
	append_overlay_segment(vertices, c3, c0, frame_color);

	const glm::vec3 center_world = transform_overlay_position(instance, center_local);
	append_overlay_segment(vertices,
	                      transform_overlay_position(instance, center_local - u_local * half_u),
	                      transform_overlay_position(instance, center_local + u_local * half_u),
	                      cross_color);
	append_overlay_segment(vertices,
	                      transform_overlay_position(instance, center_local - v_local * half_v),
	                      transform_overlay_position(instance, center_local + v_local * half_v),
	                      cross_color);
	append_overlay_segment(vertices,
	                      center_world,
	                      transform_overlay_position(instance, center_local + safe_normalize(glm::cross(u_local, v_local), glm::vec3(0.0f, 0.0f, 1.0f)) * std::min(half_u, half_v) * 0.35f),
	                      cross_color);
}

void append_overlay_cylinder_wireframe(std::vector<PreviewOverlayVertex> *vertices,
                                       const PrimitiveInstance &instance,
                                       const glm::vec3 &center_local,
                                       const std::array<int, 3> &axis_vector,
                                       float axial_length,
                                       float radial_u,
                                       float radial_v,
                                       const ImVec4 &ring_color,
                                       const ImVec4 &spine_color,
                                       int ring_count = 2)
{
	if (vertices == nullptr || axial_length <= 0.0001f || radial_u <= 0.0001f || radial_v <= 0.0001f) {
		return;
	}

	glm::vec3 axis_local;
	glm::vec3 u_local;
	glm::vec3 v_local;
	local_basis_from_axis(axis_vector, &axis_local, &u_local, &v_local);
	const float half_length = axial_length * 0.5f;
	const int resolved_ring_count = std::max(2, ring_count);
	for (int ring_index = 0; ring_index < resolved_ring_count; ++ring_index) {
		const float t = resolved_ring_count == 1 ? 0.0f :
			(static_cast<float>(ring_index) / static_cast<float>(resolved_ring_count - 1)) * 2.0f - 1.0f;
		append_overlay_ring(vertices,
		                   instance,
		                   center_local + axis_local * (half_length * t),
		                   u_local,
		                   v_local,
		                   radial_u,
		                   radial_v,
		                   ring_color,
		                   24);
	}

	for (int spoke_index = 0; spoke_index < 4; ++spoke_index) {
		const float angle = (6.28318530718f * static_cast<float>(spoke_index)) / 4.0f;
		const glm::vec3 radial_offset = u_local * (std::cos(angle) * radial_u) + v_local * (std::sin(angle) * radial_v);
		append_overlay_segment(vertices,
		                      transform_overlay_position(instance, center_local - axis_local * half_length + radial_offset),
		                      transform_overlay_position(instance, center_local + axis_local * half_length + radial_offset),
		                      spine_color);
	}
}

void append_overlay_box(std::vector<PreviewOverlayVertex> *vertices,
                        const PrimitiveInstance &instance,
                        const LocalMeshBounds &bounds,
                        const ImVec4 &color)
{
	if (vertices == nullptr || !bounds.valid) {
		return;
	}

	const glm::vec3 corners[8] = {
		{bounds.min.x, bounds.min.y, bounds.min.z},
		{bounds.max.x, bounds.min.y, bounds.min.z},
		{bounds.max.x, bounds.max.y, bounds.min.z},
		{bounds.min.x, bounds.max.y, bounds.min.z},
		{bounds.min.x, bounds.min.y, bounds.max.z},
		{bounds.max.x, bounds.min.y, bounds.max.z},
		{bounds.max.x, bounds.max.y, bounds.max.z},
		{bounds.min.x, bounds.max.y, bounds.max.z},
	};
	const int edges[12][2] = {
		{0, 1}, {1, 2}, {2, 3}, {3, 0},
		{4, 5}, {5, 6}, {6, 7}, {7, 4},
		{0, 4}, {1, 5}, {2, 6}, {3, 7},
	};
	glm::vec3 world_corners[8];
	for (int index = 0; index < 8; ++index) {
		world_corners[index] = transform_overlay_position(instance, corners[index]);
	}
	for (const auto &edge : edges) {
		append_overlay_segment(vertices, world_corners[edge[0]], world_corners[edge[1]], color);
	}
}

const StlCatalogEntry *catalog_entry_for_instance_type(const std::string &instance_type)
{
	if (instance_type.size() <= 4 || instance_type.rfind("STL.", 0) != 0) {
		return nullptr;
	}
	const auto entry_it = stl_catalog_lookup.find(lowercase_copy(instance_type.substr(4)));
	if (entry_it != stl_catalog_lookup.end()) {
		return &entry_it->second;
	}
	return nullptr;
}

std::vector<PreviewOverlayVertex> build_connection_overlay_vertices(Context *context)
{
	std::vector<PreviewOverlayVertex> vertices;
	if (context == nullptr || !connection_overlay_visible()) {
		return vertices;
	}

	for (const PrimitiveInstance &instance : context->primitive_instances) {
		if (!Mesh::isStlInstanceType(instance.type)) {
			continue;
		}
		const StlCatalogEntry *entry = catalog_entry_for_instance_type(instance.type);
		if (entry == nullptr) {
			continue;
		}

		const Mesh &mesh = Mesh::getSharedInstance(instance.type);
		const LocalMeshBounds bounds = local_bounds_for_entry(*entry, mesh);
		const float max_extent = std::max(bounds.max_extent, 0.25f);
		const float default_axis_length = std::clamp(max_extent * 0.34f, 0.18f, std::max(4.0f, max_extent * 1.25f));
		const float marker_size = std::clamp(max_extent * 0.09f, 0.035f, 0.55f);
		const float surface_offset = std::clamp(max_extent * 0.015f, 0.006f, 0.18f);

		if (connection_bounds_visible()) {
			append_overlay_box(&vertices, instance, bounds, color_from_hex(0xe984ff, 0.34f));
		}

		if (connection_axes_visible()) {
			for (const StlCatalogEntry::ConnectionPoint &point : entry->connection_points) {
				const StlCatalogEntry::ConnectionSurface *surface = find_connection_surface(*entry, point.name);
				const ConnectionSemanticKind kind = classify_connection_kind(*entry, &point, surface, nullptr);
				const ConnectionFlowRole flow = classify_connection_flow(*entry, &point, surface, nullptr, kind);
				const ImVec4 axis_color = color_for_connection_kind(kind, 0.96f);
				const ImVec4 flow_color = color_for_flow_role(flow, 0.98f);
				const glm::vec3 center_local = to_vec3(point.center);
				const glm::vec3 axis_local = local_axis_direction(point.axis_vector);
				const glm::vec3 axis_world = transform_overlay_direction(instance, axis_local);
				float axis_length = 0.0f;
				if (surface != nullptr && has_nonzero_size(surface->bounds_numeric.size)) {
					axis_length = axial_extent_from_size(surface->axis_vector, surface->bounds_numeric.size);
				}
				if (axis_length <= 0.0001f && has_nonzero_size(point.extents_numeric.size)) {
					axis_length = max_size_component(point.extents_numeric.size);
				}
				if (axis_length <= 0.0001f && point.diameter_numeric > 0.0f) {
					axis_length = point.diameter_numeric * 3.2f;
				}
				if (axis_length <= 0.0001f) {
					axis_length = default_axis_length;
				}
				axis_length = std::clamp(axis_length, marker_size * 3.0f, std::max(default_axis_length * 2.0f, marker_size * 3.0f));

				const glm::vec3 start_local = center_local - axis_local * (axis_length * 0.5f);
				const glm::vec3 end_local = center_local + axis_local * (axis_length * 0.5f);
				const glm::vec3 center_world = transform_overlay_position(instance, center_local);
				const glm::vec3 start = transform_overlay_position(instance, start_local);
				const glm::vec3 end = transform_overlay_position(instance, end_local);
				append_overlay_segment(&vertices, start, end, axis_color);

				append_overlay_segment(&vertices,
				                      transform_overlay_position(instance, center_local - glm::vec3(marker_size, 0.0f, 0.0f)),
				                      transform_overlay_position(instance, center_local + glm::vec3(marker_size, 0.0f, 0.0f)),
				                      axis_color);
				append_overlay_segment(&vertices,
				                      transform_overlay_position(instance, center_local - glm::vec3(0.0f, marker_size, 0.0f)),
				                      transform_overlay_position(instance, center_local + glm::vec3(0.0f, marker_size, 0.0f)),
				                      axis_color);
				append_overlay_segment(&vertices,
				                      transform_overlay_position(instance, center_local - glm::vec3(0.0f, 0.0f, marker_size)),
				                      transform_overlay_position(instance, center_local + glm::vec3(0.0f, 0.0f, marker_size)),
				                      axis_color);

				if (point.diameter_numeric > 0.0001f) {
					glm::vec3 unused_axis;
					glm::vec3 u_local;
					glm::vec3 v_local;
					local_basis_from_axis(point.axis_vector, &unused_axis, &u_local, &v_local);
					append_overlay_ring(&vertices,
					                   instance,
					                   center_local,
					                   u_local,
					                   v_local,
					                   point.diameter_numeric * 0.5f,
					                   point.diameter_numeric * 0.5f,
					                   flow == ConnectionFlowRole::Static ? axis_color : flow_color,
					                   20);
				}

				if (flow == ConnectionFlowRole::Output || flow == ConnectionFlowRole::Bidirectional) {
					append_overlay_arrowhead(&vertices,
					                        end,
					                        axis_world,
					                        transform_overlay_direction(instance, glm::vec3(0.0f, 1.0f, 0.0f)),
					                        marker_size * 1.3f,
					                        flow_color);
					append_overlay_arrowhead(&vertices,
					                        start,
					                        -axis_world,
					                        transform_overlay_direction(instance, glm::vec3(0.0f, 1.0f, 0.0f)),
					                        marker_size * 1.3f,
					                        flow_color);
				}
				if (flow == ConnectionFlowRole::Input) {
					append_overlay_arrowhead(&vertices,
					                        center_world + axis_world * (marker_size * 0.6f),
					                        -axis_world,
					                        transform_overlay_direction(instance, glm::vec3(0.0f, 1.0f, 0.0f)),
					                        marker_size * 1.15f,
					                        flow_color);
					append_overlay_arrowhead(&vertices,
					                        center_world - axis_world * (marker_size * 0.6f),
					                        axis_world,
					                        transform_overlay_direction(instance, glm::vec3(0.0f, 1.0f, 0.0f)),
					                        marker_size * 1.15f,
					                        flow_color);
				}
			}
		}

		if (connection_surfaces_visible()) {
			for (const StlCatalogEntry::ConnectionSurface &surface : entry->connection_surfaces) {
				const StlCatalogEntry::ConnectionPoint *point =
					find_connection_point(*entry, surface.source_connection_point);
				const ConnectionSemanticKind kind = classify_connection_kind(*entry, point, &surface, nullptr);
				const ConnectionFlowRole flow = classify_connection_flow(*entry, point, &surface, nullptr, kind);
				const std::array<int, 3> &resolved_axis_vector =
					point != nullptr && is_cardinal_axis_vector(point->axis_vector) ? point->axis_vector : surface.axis_vector;
				const std::string resolved_plane =
					is_cardinal_axis_vector(resolved_axis_vector) ? plane_from_axis_vector(resolved_axis_vector) : surface.plane;
				const ImVec4 plane_color = color_for_plane(resolved_plane, 0.96f);
				const ImVec4 domain_color = color_for_connection_kind(kind, 0.9f);
				const ImVec4 flow_color = color_for_flow_role(flow, 0.96f);
				const glm::vec3 center_local = point != nullptr ? to_vec3(point->center) : to_vec3(surface.center);
				const bool cylindrical_surface =
					surface.shape.find("cylindrical") != std::string::npos &&
					surface.shape.find("pattern") == std::string::npos;
				if (cylindrical_surface && has_nonzero_size(surface.bounds_numeric.size)) {
					const float axial_length = std::max(0.0f, axial_extent_from_size(resolved_axis_vector, surface.bounds_numeric.size));
					std::pair<float, float> radial = radial_extents_from_size(resolved_axis_vector, surface.bounds_numeric.size);
					if (surface.diameter_numeric > 0.0001f) {
						radial.first = surface.diameter_numeric;
						radial.second = surface.diameter_numeric;
					}
					const float radius_u = std::max(radial.first * 0.5f, 0.02f);
					const float radius_v = std::max(radial.second * 0.5f, 0.02f);
					int ring_count = 2;
					if (surface.thread_pitch_numeric > 0.0001f && axial_length > 0.0001f) {
						ring_count = std::clamp(static_cast<int>(std::ceil(axial_length / surface.thread_pitch_numeric)) + 1,
						                        3,
						                        14);
					}
					append_overlay_cylinder_wireframe(&vertices,
					                                 instance,
					                                 center_local,
					                                 resolved_axis_vector,
					                                 std::max(axial_length, marker_size * 3.0f),
					                                 radius_u,
					                                 radius_v,
					                                 plane_color,
					                                 domain_color,
					                                 ring_count);

					const glm::vec3 axis_world =
						transform_overlay_direction(instance, local_axis_direction(resolved_axis_vector));
					const glm::vec3 center_world = transform_overlay_position(instance, center_local);
					if (flow == ConnectionFlowRole::Output || flow == ConnectionFlowRole::Bidirectional) {
						append_overlay_arrowhead(&vertices,
						                        center_world + axis_world * (std::max(axial_length, marker_size * 3.0f) * 0.5f),
						                        axis_world,
						                        transform_overlay_direction(instance, glm::vec3(0.0f, 1.0f, 0.0f)),
						                        marker_size * 1.15f,
						                        flow_color);
					}
					if (flow == ConnectionFlowRole::Input || flow == ConnectionFlowRole::Bidirectional) {
						append_overlay_arrowhead(&vertices,
						                        center_world - axis_world * (std::max(axial_length, marker_size * 3.0f) * 0.5f),
						                        -axis_world,
						                        transform_overlay_direction(instance, glm::vec3(0.0f, 1.0f, 0.0f)),
						                        marker_size * 1.15f,
						                        flow_color);
					}
					continue;
				}

				glm::vec3 u_local;
				glm::vec3 v_local;
				glm::vec3 n_local;
				local_basis_from_surface_axis(resolved_axis_vector, resolved_plane, &u_local, &v_local, &n_local);
				const std::pair<float, float> plane_extents = has_nonzero_size(surface.bounds_numeric.size)
					                                              ? plane_extents_from_size(resolved_plane, surface.bounds_numeric.size)
					                                              : std::make_pair(bounds.size.x * 0.42f, bounds.size.y * 0.42f);
				const float half_u = std::max(plane_extents.first * 0.5f, marker_size * 1.2f);
				const float half_v = std::max(plane_extents.second * 0.5f, marker_size * 1.2f);
				const glm::vec3 offset_local = center_local + n_local * surface_offset;
				append_overlay_plane_frame(&vertices,
				                          instance,
				                          offset_local,
				                          u_local,
				                          v_local,
				                          half_u,
				                          half_v,
				                          plane_color,
				                          domain_color);

				const glm::vec3 center_world = transform_overlay_position(instance, offset_local);
				const glm::vec3 normal_world = transform_overlay_direction(instance, n_local);
				append_overlay_segment(&vertices,
				                      center_world,
				                      center_world + normal_world * (marker_size * 2.3f),
				                      domain_color);
				if (flow == ConnectionFlowRole::Output || flow == ConnectionFlowRole::Bidirectional) {
					append_overlay_arrowhead(&vertices,
					                        center_world + normal_world * (marker_size * 2.3f),
					                        normal_world,
					                        transform_overlay_direction(instance, glm::vec3(0.0f, 1.0f, 0.0f)),
					                        marker_size,
					                        flow_color);
				}
				if (flow == ConnectionFlowRole::Input) {
					append_overlay_arrowhead(&vertices,
					                        center_world + normal_world * (marker_size * 0.6f),
					                        -normal_world,
					                        transform_overlay_direction(instance, glm::vec3(0.0f, 1.0f, 0.0f)),
					                        marker_size,
					                        flow_color);
				}
			}
		}

		if (connection_features_visible()) {
			for (const StlCatalogEntry::DetectedConnection &feature : entry->scad_detected_connections) {
				const ConnectionSemanticKind kind = classify_connection_kind(*entry, nullptr, nullptr, &feature);
				const ConnectionFlowRole flow = classify_connection_flow(*entry, nullptr, nullptr, &feature, kind);
				const ImVec4 domain_color = color_for_connection_kind(kind, 0.84f);
				const ImVec4 plane_color = color_for_plane(feature.plane, 0.92f);
				const ImVec4 flow_color = color_for_flow_role(flow, 0.96f);
				const glm::vec3 center_local = to_vec3(feature.center);

				std::array<float, 3> feature_size = feature.bounds_numeric.size;
				if (!has_nonzero_size(feature_size)) {
					const float diameter =
						feature.outer_diameter_numeric > 0.0001f ? feature.outer_diameter_numeric : feature.diameter_numeric;
					const float length = feature.length_numeric > 0.0001f ? feature.length_numeric : default_axis_length;
					const int axis_index = axis_index_from_vector(feature.axis_vector);
					if (axis_index == 0) {
						feature_size = {length, diameter, diameter};
					} else if (axis_index == 1) {
						feature_size = {diameter, length, diameter};
					} else {
						feature_size = {diameter, diameter, length};
					}
				}

				const bool cylindrical_feature =
					feature.primitive == "circle" || feature.primitive == "poly_circle" ||
					feature.primitive == "cylinder" || feature.primitive == "poly_cylinder" ||
					feature.primitive == "rounded_cylinder" || feature.primitive == "tube" ||
					feature.feature_role == "hole" || feature.feature_role == "threaded_hole" ||
					feature.feature_role == "protrusion" || feature.feature_role == "tube" ||
					feature.feature_role == "thread";

				if (cylindrical_feature) {
					const float axial_length = std::max(0.0f, feature.length_numeric > 0.0001f
						                                              ? feature.length_numeric
						                                              : axial_extent_from_size(feature.axis_vector, feature_size));
					std::pair<float, float> radial = radial_extents_from_size(feature.axis_vector, feature_size);
					if (feature.outer_diameter_numeric > 0.0001f) {
						radial.first = feature.outer_diameter_numeric;
						radial.second = feature.outer_diameter_numeric;
					} else if (feature.diameter_numeric > 0.0001f) {
						radial.first = feature.diameter_numeric;
						radial.second = feature.diameter_numeric;
					}

					const float radius_u = std::max(radial.first * 0.5f, 0.02f);
					const float radius_v = std::max(radial.second * 0.5f, 0.02f);
					int ring_count = 2;
					if (feature.thread_pitch_numeric > 0.0001f && axial_length > 0.0001f) {
						ring_count = std::clamp(static_cast<int>(std::ceil(axial_length / feature.thread_pitch_numeric)) + 1,
						                        3,
						                        12);
					}
					append_overlay_cylinder_wireframe(&vertices,
					                                 instance,
					                                 center_local,
					                                 feature.axis_vector,
					                                 std::max(axial_length, marker_size * 2.5f),
					                                 radius_u,
					                                 radius_v,
					                                 plane_color,
					                                 domain_color,
					                                 ring_count);

					if (feature.outer_diameter_numeric > 0.0001f && feature.inner_diameter_numeric > 0.0001f) {
						glm::vec3 axis_local;
						glm::vec3 u_local;
						glm::vec3 v_local;
						local_basis_from_axis(feature.axis_vector, &axis_local, &u_local, &v_local);
						const float half_length = std::max(axial_length, marker_size * 2.5f) * 0.5f;
						append_overlay_ring(&vertices,
						                   instance,
						                   center_local - axis_local * half_length,
						                   u_local,
						                   v_local,
						                   feature.inner_diameter_numeric * 0.5f,
						                   feature.inner_diameter_numeric * 0.5f,
						                   flow_color,
						                   20);
						append_overlay_ring(&vertices,
						                   instance,
						                   center_local + axis_local * half_length,
						                   u_local,
						                   v_local,
						                   feature.inner_diameter_numeric * 0.5f,
						                   feature.inner_diameter_numeric * 0.5f,
						                   flow_color,
						                   20);
					}

					const glm::vec3 axis_world =
						transform_overlay_direction(instance, local_axis_direction(feature.axis_vector));
					const glm::vec3 center_world = transform_overlay_position(instance, center_local);
					append_overlay_segment(&vertices,
					                      center_world - axis_world * (std::max(axial_length, marker_size * 2.5f) * 0.5f),
					                      center_world + axis_world * (std::max(axial_length, marker_size * 2.5f) * 0.5f),
					                      domain_color);
					if (flow == ConnectionFlowRole::Output || flow == ConnectionFlowRole::Bidirectional) {
						append_overlay_arrowhead(&vertices,
						                        center_world + axis_world * (std::max(axial_length, marker_size * 2.5f) * 0.5f),
						                        axis_world,
						                        transform_overlay_direction(instance, glm::vec3(0.0f, 1.0f, 0.0f)),
						                        marker_size,
						                        flow_color);
					}
					if (flow == ConnectionFlowRole::Input || flow == ConnectionFlowRole::Bidirectional) {
						append_overlay_arrowhead(&vertices,
						                        center_world - axis_world * (std::max(axial_length, marker_size * 2.5f) * 0.5f),
						                        -axis_world,
						                        transform_overlay_direction(instance, glm::vec3(0.0f, 1.0f, 0.0f)),
						                        marker_size,
						                        flow_color);
					}
					continue;
				}

				glm::vec3 u_local;
				glm::vec3 v_local;
				glm::vec3 n_local;
				local_basis_from_plane(feature.plane, &u_local, &v_local, &n_local);
				const std::pair<float, float> plane_extents =
					plane_extents_from_size(feature.plane, feature_size);
				append_overlay_plane_frame(&vertices,
				                          instance,
				                          center_local + n_local * surface_offset,
				                          u_local,
				                          v_local,
				                          std::max(plane_extents.first * 0.5f, marker_size),
				                          std::max(plane_extents.second * 0.5f, marker_size),
				                          plane_color,
				                          domain_color);
			}
		}
	}

	return vertices;
}

ImVec4 color_for_spatial_overlay_role(SpatialOverlayColorRole role)
{
	switch (role) {
	case SpatialOverlayColorRole::Validated: return color_from_hex(0x55d187, 0.94f);
	case SpatialOverlayColorRole::Pending: return color_from_hex(0xe7c95f, 0.92f);
	case SpatialOverlayColorRole::Interface: return color_from_hex(0x53d7e8, 0.96f);
	case SpatialOverlayColorRole::Connection: return color_from_hex(0x5897ff, 0.94f);
	case SpatialOverlayColorRole::Clearance: return color_from_hex(0xffa33e, 0.98f);
	case SpatialOverlayColorRole::Constraint: return color_from_hex(0xe76cff, 0.96f);
	case SpatialOverlayColorRole::BuildingFunction: return color_from_hex(0xa982ff, 0.98f);
	case SpatialOverlayColorRole::BuildingServiceFlow: return color_from_hex(0x22d3b6, 0.98f);
	case SpatialOverlayColorRole::BuildingRequirement: return color_from_hex(0x7ee787, 0.98f);
	case SpatialOverlayColorRole::BuildingPendingEvidence: return color_from_hex(0xffc857, 0.98f);
	case SpatialOverlayColorRole::Invalid: return color_from_hex(0xff5b63, 0.98f);
	case SpatialOverlayColorRole::FrameX: return color_from_hex(0xf15b64, 0.98f);
	case SpatialOverlayColorRole::FrameY: return color_from_hex(0x62cf73, 0.98f);
	case SpatialOverlayColorRole::FrameZ: return color_from_hex(0x5d91ff, 0.98f);
	}
	return color_from_hex(0xffffff, 0.9f);
}

void append_spatial_overlay_vertices(
	std::vector<PreviewOverlayVertex> *vertices,
	Context *context)
{
	if (vertices == nullptr || context == nullptr || !spatial_overlay_visible() ||
	    !context->spatialBuildingModel()) {
		return;
	}
	const SpatialBuildingModel &model = *context->spatialBuildingModel();
	SpatialOverlayEvidenceRequest request;
	request.selected_object_id =
		current_preview_session().selection.selected_spatial_object_id.empty()
			? model.containmentTree().rootObjectId().value()
			: current_preview_session().selection.selected_spatial_object_id;
	request.object_frames_visible = spatial_object_frames_visible();
	request.interfaces_visible = spatial_interfaces_visible();
	request.connections_visible = spatial_connections_visible();
	request.constraints_visible = spatial_constraints_visible();
	request.contacts_visible = spatial_contacts_visible();
	request.clearances_visible = spatial_clearances_visible();
	request.bounds_visible = spatial_bounds_visible();
	request.building_function_allocations_visible =
		building_function_allocations_visible();
	request.building_service_flows_visible = building_service_flows_visible();
	request.building_requirement_status_visible = building_requirement_status_visible();
	request.building_pending_evidence_visible = building_pending_evidence_visible();

	const PrimitiveBounds scene_bounds = context->getSceneBounds();
	const float scene_extent = scene_bounds.valid
		? std::max({scene_bounds.half_extents.x,
		            scene_bounds.half_extents.y,
		            scene_bounds.half_extents.z}) * 2.0f
		: 1.0f;
	request.marker_length = std::clamp(scene_extent * 0.025f, 0.12f, 2.0f);

	const SpatialObjectBoundaryDerivationService boundary_derivation;
	for (const SpatialBuildingObject &object : model.objects().objects()) {
		const SpatialObjectId &object_id = object.identity().objectId();
		const SpatialBoundaryModel boundary = boundary_derivation.derive(
			object, model.bindingsFor(object_id), *context);
		const AxisAlignedBoundingBoundary *aabb = boundary.axisAlignedBoundary();
		if (aabb != nullptr) {
			request.object_world_bounds.emplace(object_id, aabb->bounds());
		}
	}

	for (const SpatialOverlaySegment &segment :
	     spatial_overlay_evidence_service.build(
		     model, context->smallModernBuildingModel().get(), request)) {
		append_overlay_segment(
			vertices,
			segment.start(),
			segment.end(),
			color_for_spatial_overlay_role(segment.colorRole()));
	}
}

std::vector<glm::vec3> axial_profile_ring_positions(
	const AxialProfileShapeSpecification &specification,
	const AxialProfileLevel &level)
{
	const AxialProfilePolygon &profile =
		specification.profiles()[level.profileIndex()];
	const float radians = glm::radians(level.rotationDegrees());
	const float cosine = std::cos(radians);
	const float sine = std::sin(radians);
	std::vector<glm::vec3> positions;
	positions.reserve(profile.vertices().size());
	for (const glm::vec2 &vertex : profile.vertices()) {
		const glm::vec2 scaled(vertex.x * level.scale().x,
		                       vertex.y * level.scale().y);
		positions.emplace_back(
			level.center().x + cosine * scaled.x - sine * scaled.y,
			level.axialPosition(),
			level.center().y + sine * scaled.x + cosine * scaled.y);
	}
	return positions;
}

ImVec4 axial_transition_overlay_color(AxialTransitionKind transition)
{
	switch (transition) {
	case AxialTransitionKind::Initial:
		return color_from_hex(0xf2f4f8, 0.92f);
	case AxialTransitionKind::Hold:
		return color_from_hex(0x66a7ff, 0.92f);
	case AxialTransitionKind::Linear:
		return color_from_hex(0x66d49b, 0.92f);
	case AxialTransitionKind::Step:
		return color_from_hex(0xffa657, 0.96f);
	}
	return color_from_hex(0xffffff, 0.9f);
}

void append_axial_profile_overlay_vertices(
	std::vector<PreviewOverlayVertex> *vertices,
	Context *context)
{
	if (vertices == nullptr || context == nullptr ||
	    !axial_profile_overlay_visible()) {
		return;
	}
	for (const PrimitiveInstance &instance : context->primitive_instances) {
		const auto *specification =
			dynamic_cast<const AxialProfileShapeSpecification *>(
				instance.shape_specification.get());
		if (specification == nullptr) continue;

		glm::vec3 previous_center(0.0f);
		bool has_previous_center = false;
		for (const AxialProfileLevel &level : specification->levels()) {
			const ImVec4 color = axial_transition_overlay_color(level.transition());
			const std::vector<glm::vec3> ring =
				axial_profile_ring_positions(*specification, level);
			for (std::size_t vertex_index = 0;
			     vertex_index < ring.size();
			     ++vertex_index) {
				append_overlay_segment(
					vertices,
					transform_overlay_position(instance, ring[vertex_index]),
					transform_overlay_position(
						instance, ring[(vertex_index + 1) % ring.size()]),
					color);
			}

			const glm::vec3 center_local(
				level.center().x, level.axialPosition(), level.center().y);
			const glm::vec3 center_world =
				transform_overlay_position(instance, center_local);
			const float marker_size = 0.035f;
			append_overlay_segment(
				vertices,
				transform_overlay_position(
					instance, center_local + glm::vec3(-marker_size, 0.0f, 0.0f)),
				transform_overlay_position(
					instance, center_local + glm::vec3(marker_size, 0.0f, 0.0f)),
				color);
			append_overlay_segment(
				vertices,
				transform_overlay_position(
					instance, center_local + glm::vec3(0.0f, 0.0f, -marker_size)),
				transform_overlay_position(
					instance, center_local + glm::vec3(0.0f, 0.0f, marker_size)),
				color);
			if (has_previous_center) {
				append_overlay_segment(
					vertices, previous_center, center_world,
					color_from_hex(0xd7dde8, 0.72f));
			}
			previous_center = center_world;
			has_previous_center = true;
		}
	}
}

void append_collision_particle_overlay_vertices(std::vector<PreviewOverlayVertex> *vertices,
                                                Context *context)
{
	if (vertices == nullptr || context == nullptr) {
		return;
	}

	for (const CollisionParticle &particle : context->getCollisionParticles()) {
		if (particle.initial_lifetime <= 0.0f || particle.lifetime <= 0.0f) {
			continue;
		}

		glm::vec3 direction = particle.velocity;
		const float direction_length2 = glm::dot(direction, direction);
		if (direction_length2 <= 0.000001f) {
			direction = glm::vec3(0.0f, 1.0f, 0.0f);
		} else {
			direction /= std::sqrt(direction_length2);
		}

		const float alpha = std::clamp(particle.color.a, 0.0f, 1.0f);
		if (alpha <= 0.01f) {
			continue;
		}

		const ImVec4 color(particle.color.r, particle.color.g, particle.color.b, alpha);
		const float streak_length = std::max(particle.size, 0.03f);
		append_overlay_segment(vertices,
		                      particle.position,
		                      particle.position - (direction * streak_length),
		                      color);
	}
}

void draw_connection_legend_chip(const char *label, const ImVec4 &color)
{
	ImGui::ColorButton(label,
	                   color,
	                   ImGuiColorEditFlags_NoTooltip |
	                       ImGuiColorEditFlags_NoDragDrop |
	                       ImGuiColorEditFlags_NoBorder,
	                   ImVec2(12.0f, 12.0f));
	ImGui::SameLine(0.0f, 6.0f);
	ImGui::TextUnformatted(label);
}

std::vector<std::string> breakup_into_lines(std::string input, std::string delimiter);
bool input_text_string(const char *label, std::string *text, ImGuiInputTextFlags flags = 0);
void request_editor_focus_restore();
void commit_editor_text(EditorWorkspaceSession *app_state, const std::string &text);

constexpr float kPreviewNearPlane = 0.01f;
constexpr float kPreviewFarPlane = 100.0f;
constexpr float kPreviewMinCameraDistance = 1.25f;
constexpr float kPreviewMaxCameraDistance = 80.0f;

glm::vec3 scaled_preview_target()
{
	return glm::vec3(preview_target_x_coordinate(), preview_target_y_coordinate(), preview_target_z_coordinate()) * preview_scale_factor();
}

PreviewInteractionCameraState build_preview_camera_state(const ImVec2 &viewport_size)
{
	const float width = std::max(viewport_size.x, 1.0f);
	const float height = std::max(viewport_size.y, 1.0f);
	const float angle_radians = glm::radians(-preview_azimuth_degrees());
	const float elevation_radians = glm::radians(preview_elevation_degrees());
	const glm::vec3 orbit_direction(std::cos(elevation_radians) * std::cos(angle_radians),
	                                std::sin(elevation_radians),
	                                std::cos(elevation_radians) * std::sin(angle_radians));

	PreviewInteractionCameraState camera;
	camera.target = glm::vec3(preview_target_x_coordinate(), preview_target_y_coordinate(), preview_target_z_coordinate());
	camera.scaled_target = scaled_preview_target();
	camera.position = camera.scaled_target + orbit_direction * preview_camera_distance_value();
	camera.view_direction = glm::normalize(camera.position - camera.scaled_target);
	camera.view = glm::lookAt(
		camera.position,
		camera.scaled_target,
		current_preview_session().camera.upDirection());
	const float far_plane =
		std::max(kPreviewFarPlane, preview_camera_distance_value() * 4.5f + 24.0f);
	camera.projection = glm::perspective(
	                                     current_preview_session().projection.verticalFieldOfViewRadians(),
	                                     width / height,
	                                     kPreviewNearPlane,
	                                     far_plane);
	return camera;
}

PreviewLightCameraContext build_preview_light_camera_context(
	const PreviewInteractionCameraState &camera)
{
	PreviewLightCameraContext light_camera;
	light_camera.camera_position = camera.position;
	light_camera.camera_target = camera.scaled_target;
	light_camera.camera_view_direction = camera.view_direction;
	light_camera.camera_right = glm::cross(
		glm::vec3(0.0f, 1.0f, 0.0f), light_camera.camera_view_direction);
	if (glm::length(light_camera.camera_right) <= 0.0001f) {
		light_camera.camera_right = glm::vec3(1.0f, 0.0f, 0.0f);
	} else {
		light_camera.camera_right = glm::normalize(light_camera.camera_right);
	}
	light_camera.camera_up = glm::normalize(glm::cross(
		light_camera.camera_view_direction, light_camera.camera_right));
	light_camera.orbit_distance = preview_camera_distance_value();
	light_camera.scene_scale = preview_scale_factor();
	return light_camera;
}

void append_light_gizmo_overlay_vertices(
	std::vector<PreviewOverlayVertex> *vertices,
	const PreviewInteractionCameraState &camera)
{
	if (vertices == nullptr) return;
	const std::vector<LightGizmoSegment> segments = LightGizmoOverlayService().build(
		current_preview_session().lighting,
		build_preview_light_camera_context(camera));
	for (const LightGizmoSegment &segment : segments) {
		vertices->push_back(PreviewOverlayVertex{
			segment.start.x, segment.start.y, segment.start.z,
			segment.color.r, segment.color.g, segment.color.b, segment.color.a});
		vertices->push_back(PreviewOverlayVertex{
			segment.end.x, segment.end.y, segment.end.z,
			segment.color.r, segment.color.g, segment.color.b, segment.color.a});
	}
}

bool ray_intersects_aabb(const glm::vec3 &origin,
                         const glm::vec3 &direction,
                         const PrimitiveBounds &bounds,
                         float *hit_distance)
{
	float t_min = 0.0f;
	float t_max = std::numeric_limits<float>::max();

	for (int axis = 0; axis < 3; ++axis) {
		if (std::fabs(direction[axis]) <= 0.000001f) {
			if (origin[axis] < bounds.min[axis] || origin[axis] > bounds.max[axis]) {
				return false;
			}
			continue;
		}

		const float inverse_direction = 1.0f / direction[axis];
		float t0 = (bounds.min[axis] - origin[axis]) * inverse_direction;
		float t1 = (bounds.max[axis] - origin[axis]) * inverse_direction;
		if (t0 > t1) {
			std::swap(t0, t1);
		}
		t_min = std::max(t_min, t0);
		t_max = std::min(t_max, t1);
		if (t_max < t_min) {
			return false;
		}
	}

	if (hit_distance != nullptr) {
		*hit_distance = t_min;
	}
	return true;
}

bool ray_intersects_triangle(const glm::vec3 &origin,
                             const glm::vec3 &direction,
                             const CollisionTriangle &triangle,
                             float *hit_distance)
{
	const glm::vec3 a = triangle.vertices[0] * preview_scale_factor();
	const glm::vec3 b = triangle.vertices[1] * preview_scale_factor();
	const glm::vec3 c = triangle.vertices[2] * preview_scale_factor();
	const glm::vec3 edge_ab = b - a;
	const glm::vec3 edge_ac = c - a;
	const glm::vec3 p = glm::cross(direction, edge_ac);
	const float determinant = glm::dot(edge_ab, p);
	if (std::fabs(determinant) <= 0.000001f) {
		return false;
	}

	const float inverse_determinant = 1.0f / determinant;
	const glm::vec3 s = origin - a;
	const float u = glm::dot(s, p) * inverse_determinant;
	if (u < -0.00001f || u > 1.00001f) {
		return false;
	}

	const glm::vec3 q = glm::cross(s, edge_ab);
	const float v = glm::dot(direction, q) * inverse_determinant;
	if (v < -0.00001f || u + v > 1.00001f) {
		return false;
	}

	const float distance = glm::dot(edge_ac, q) * inverse_determinant;
	if (distance <= 0.00001f) {
		return false;
	}

	if (hit_distance != nullptr) {
		*hit_distance = distance;
	}
	return true;
}

bool ray_intersects_collision_geometry(const glm::vec3 &origin,
                                       const glm::vec3 &direction,
                                       const CollisionGeometry &geometry,
                                       float *hit_distance)
{
	if (!geometry.valid) {
		return false;
	}

	PrimitiveBounds scaled_bounds = geometry.bounds;
	scaled_bounds.min *= preview_scale_factor();
	scaled_bounds.max *= preview_scale_factor();
	scaled_bounds.center *= preview_scale_factor();
	scaled_bounds.half_extents *= preview_scale_factor();
	float bounds_hit_distance = 0.0f;
	if (!ray_intersects_aabb(origin, direction, scaled_bounds, &bounds_hit_distance)) {
		return false;
	}

	float nearest_triangle_distance = std::numeric_limits<float>::max();
	bool hit_triangle = false;
	for (const CollisionTriangle &triangle : geometry.triangles) {
		if (!triangle.valid) {
			continue;
		}
		float triangle_hit_distance = 0.0f;
		if (!ray_intersects_triangle(origin, direction, triangle, &triangle_hit_distance)) {
			continue;
		}
		nearest_triangle_distance = std::min(nearest_triangle_distance, triangle_hit_distance);
		hit_triangle = true;
	}

	if (hit_distance != nullptr) {
		*hit_distance = hit_triangle ? nearest_triangle_distance : bounds_hit_distance;
	}
	return true;
}

bool is_valid_preview_instance_index(Context *context, int index)
{
	return context != nullptr &&
	       index >= 0 &&
	       static_cast<std::size_t>(index) < context->primitive_instances.size() &&
	       !context->primitive_instances[static_cast<std::size_t>(index)].removed;
}

int pick_preview_instance_index(const PreviewInteractionCameraState &camera,
                                const ImRect &viewport_rect,
                                Context *context)
{
	if (context == nullptr || viewport_rect.GetWidth() <= 1.0f || viewport_rect.GetHeight() <= 1.0f) {
		return -1;
	}

	const ImVec2 mouse = ImGui::GetIO().MousePos;
	const PreviewTexturePresentation texture_presentation;
	const float viewport_fraction_x =
		(mouse.x - viewport_rect.Min.x) / viewport_rect.GetWidth();
	const float viewport_fraction_y =
		(mouse.y - viewport_rect.Min.y) / viewport_rect.GetHeight();
	const float x = texture_presentation.normalizedDeviceXFromViewportFraction(
		viewport_fraction_x);
	const float y = texture_presentation.normalizedDeviceYFromViewportFraction(
		viewport_fraction_y);
	const glm::mat4 inverse_view_projection = glm::inverse(camera.projection * camera.view);
	glm::vec4 near_point = inverse_view_projection * glm::vec4(x, y, -1.0f, 1.0f);
	glm::vec4 far_point = inverse_view_projection * glm::vec4(x, y, 1.0f, 1.0f);
	if (std::fabs(near_point.w) <= 0.000001f || std::fabs(far_point.w) <= 0.000001f) {
		return -1;
	}
	near_point /= near_point.w;
	far_point /= far_point.w;
	const glm::vec3 ray_origin(near_point.x, near_point.y, near_point.z);
	const glm::vec3 ray_direction =
		glm::normalize(glm::vec3(far_point.x - near_point.x,
		                         far_point.y - near_point.y,
		                         far_point.z - near_point.z));

	int hovered_index = -1;
	float nearest_distance = std::numeric_limits<float>::max();
	for (std::size_t index = 0; index < context->primitive_instances.size(); ++index) {
		const PrimitiveInstance &instance = context->primitive_instances[index];
		if (instance.removed) {
			continue;
		}
		const CollisionGeometry &geometry = context->getInstanceCollisionGeometry(instance);
		if (!geometry.valid) {
			continue;
		}
		float hit_distance = 0.0f;
		if (!ray_intersects_collision_geometry(ray_origin, ray_direction, geometry, &hit_distance)) {
			continue;
		}
		if (hit_distance < nearest_distance) {
			nearest_distance = hit_distance;
			hovered_index = static_cast<int>(index);
		}
	}
	return hovered_index;
}

void fit_preview_camera_to_scene(EditorWorkspaceSession *app_state, const ImVec2 &viewport_size)
{
	if (app_state == nullptr || grammar == nullptr || grammar->context == nullptr) {
		return;
	}

	const PrimitiveBounds bounds = grammar->context->getSceneBounds();
	if (!bounds.valid) {
		current_preview_session().fit_camera_pending = false;
		return;
	}
	current_preview_session().camera.fitToBounds(bounds.center.x,
	                                         bounds.center.y,
	                                         bounds.center.z,
	                                         bounds.half_extents.x,
	                                         bounds.half_extents.y,
	                                         bounds.half_extents.z,
	                                         viewport_size.x,
	                                         viewport_size.y,
	                                         current_preview_session().projection);
	current_preview_session().fit_camera_pending = false;
}

void sanitize_preview_selection(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return;
	}
	Context *context = grammar != nullptr ? grammar->context : nullptr;
	if (!is_valid_preview_instance_index(
		    context,
		    current_preview_session().selection.selected_instance_index)) {
		current_preview_session().selection.selected_instance_index = -1;
	}
	if (!is_valid_preview_instance_index(
		    context,
		    current_preview_session().selection.hovered_instance_index)) {
		current_preview_session().selection.hovered_instance_index = -1;
	}
}

std::vector<PreviewOutlineBatch> build_preview_outline_batches(Context *context,
                                                               const EditorWorkspaceSession *app_state)
{
	std::vector<PreviewOutlineBatch> batches;
	if (context == nullptr || app_state == nullptr) {
		return batches;
	}
	if (!app_state->preview.interaction_settings.picking_and_highlighting_enabled) {
		return batches;
	}

	const auto base_vertices = build_base_vertex_data();
	const auto append_batch = [&](int index, const glm::vec4 &color, float width) {
		if (!is_valid_preview_instance_index(context, index)) {
			return;
		}
		PreviewOutlineBatch batch;
		batch.color = color;
		batch.width = width;
		context->buildInstanceBuffer(context->primitive_instances[static_cast<std::size_t>(index)],
		                             base_vertices.data(),
		                             &batch.vertices);
		if (!batch.vertices.empty()) {
			batches.push_back(std::move(batch));
		}
	};

	const float hovered_width = std::max(0.022f, preview_camera_distance_value() * 0.0056f);
	const float selected_width = std::max(0.032f, preview_camera_distance_value() * 0.0078f);
	append_batch(current_preview_session().selection.hovered_instance_index,
	             glm::vec4(0.31f, 0.70f, 1.00f, 0.92f),
	             hovered_width);
	if (current_preview_session().selection.selected_instance_index !=
	    current_preview_session().selection.hovered_instance_index) {
		append_batch(current_preview_session().selection.selected_instance_index,
		             glm::vec4(1.00f, 0.66f, 0.28f, 0.98f),
		             selected_width);
	} else if (!batches.empty()) {
		batches.front().color = glm::vec4(1.00f, 0.72f, 0.34f, 0.98f);
		batches.front().width = selected_width;
	}
	return batches;
}

ImVec2 project_view_cube_point(const glm::vec3 &point,
                               const glm::vec3 &right,
                               const glm::vec3 &up,
                               const ImVec2 &center,
                               float scale)
{
	return ImVec2(center.x - glm::dot(point, right) * scale,
	              center.y - glm::dot(point, up) * scale);
}

bool point_in_convex_quad(const std::array<ImVec2, 4> &quad, const ImVec2 &point)
{
	float previous_cross = 0.0f;
	for (std::size_t index = 0; index < quad.size(); ++index) {
		const ImVec2 &a = quad[index];
		const ImVec2 &b = quad[(index + 1) % quad.size()];
		const ImVec2 edge(b.x - a.x, b.y - a.y);
		const ImVec2 to_point(point.x - a.x, point.y - a.y);
		const float cross = edge.x * to_point.y - edge.y * to_point.x;
		if (std::fabs(cross) <= 0.001f) {
			continue;
		}
		if (previous_cross == 0.0f) {
			previous_cross = cross;
			continue;
		}
		if ((cross > 0.0f) != (previous_cross > 0.0f)) {
			return false;
		}
	}
	return true;
}

void snap_camera_to_view_cube_face(const glm::vec3 &normal)
{
	const std::optional<PreviewOrientation> orientation =
		preview_orientation_control.orientationForNormal(normal.x, normal.y, normal.z);
	if (orientation.has_value()) {
		current_preview_session().camera.orientTo(*orientation);
	}
}

bool draw_preview_view_cube(const ImRect &preview_rect, bool *capture_pointer)
{
	if (capture_pointer != nullptr) {
		*capture_pointer = false;
	}

	const float preview_width = preview_rect.GetWidth();
	const float preview_height = preview_rect.GetHeight();
	if (preview_width <= 0.0f || preview_height <= 0.0f) {
		return false;
	}

	const float cube_size =
		std::clamp(std::min(preview_width, preview_height) * 0.19f, 78.0f, 108.0f);
	const ImRect cube_rect(ImVec2(preview_rect.Max.x - cube_size - 18.0f, preview_rect.Min.y + 18.0f),
	                       ImVec2(preview_rect.Max.x - 18.0f, preview_rect.Min.y + cube_size + 18.0f));
	const ImVec2 cube_center((cube_rect.Min.x + cube_rect.Max.x) * 0.5f,
	                         (cube_rect.Min.y + cube_rect.Max.y) * 0.5f);
	const float cube_scale = cube_size * 0.27f;
	const glm::vec3 view_direction = -current_preview_session().camera.forwardDirection();
	const glm::vec3 right = current_preview_session().camera.rightDirection();
	const glm::vec3 up = current_preview_session().camera.upDirection();
	std::array<ViewCubeFace, 6> faces{{
		{"+X", glm::vec3(1.0f, 0.0f, 0.0f), {{{1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, -1.0f, 1.0f}}}, 0xd98564},
		{"-X", glm::vec3(-1.0f, 0.0f, 0.0f), {{{-1.0f, -1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f}, {-1.0f, 1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f}}}, 0xbb6c4a},
		{"+Y", glm::vec3(0.0f, 1.0f, 0.0f), {{{-1.0f, 1.0f, -1.0f}, {-1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, -1.0f}}}, 0x89aa76},
		{"-Y", glm::vec3(0.0f, -1.0f, 0.0f), {{{-1.0f, -1.0f, 1.0f}, {-1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, -1.0f}, {1.0f, -1.0f, 1.0f}}}, 0x688857},
		{"+Z", glm::vec3(0.0f, 0.0f, 1.0f), {{{-1.0f, -1.0f, 1.0f}, {1.0f, -1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {-1.0f, 1.0f, 1.0f}}}, 0x74a9d2},
		{"-Z", glm::vec3(0.0f, 0.0f, -1.0f), {{{1.0f, -1.0f, -1.0f}, {-1.0f, -1.0f, -1.0f}, {-1.0f, 1.0f, -1.0f}, {1.0f, 1.0f, -1.0f}}}, 0x5f88b0}
	}};
	std::vector<std::size_t> visible_faces;
	visible_faces.reserve(faces.size());
	for (std::size_t index = 0; index < faces.size(); ++index) {
		faces[index].visible = glm::dot(faces[index].normal, view_direction) > 0.01f;
		if (!faces[index].visible) {
			continue;
		}
		float depth = 0.0f;
		for (const glm::vec3 &corner : faces[index].corners) {
			depth += glm::dot(corner, view_direction);
		}
		faces[index].depth = depth / static_cast<float>(faces[index].corners.size());
		visible_faces.push_back(index);
	}
	std::sort(visible_faces.begin(),
	          visible_faces.end(),
	          [&](std::size_t lhs, std::size_t rhs) {
		          return faces[lhs].depth < faces[rhs].depth;
	          });

	const bool cube_hovered = ImGui::IsMouseHoveringRect(cube_rect.Min, cube_rect.Max);
	if (capture_pointer != nullptr) {
		*capture_pointer = cube_hovered;
	}
	ImDrawList *draw_list = ImGui::GetWindowDrawList();
	draw_list->AddRectFilled(cube_rect.Min,
	                         cube_rect.Max,
	                         ImGui::GetColorU32(color_from_hex(0xfffcf8, 0.90f)),
	                         14.0f);
	draw_list->AddRect(cube_rect.Min,
	                   cube_rect.Max,
	                   ImGui::GetColorU32(color_from_hex(0xd8cfd8, 0.96f)),
	                   14.0f,
	                   0,
	                   1.0f);

	const ImVec2 mouse = ImGui::GetIO().MousePos;
	int hovered_face_index = -1;
	for (auto iter = visible_faces.rbegin(); iter != visible_faces.rend(); ++iter) {
		const std::size_t face_index = *iter;
		std::array<ImVec2, 4> polygon{};
		for (std::size_t corner_index = 0; corner_index < faces[face_index].corners.size(); ++corner_index) {
			polygon[corner_index] =
				project_view_cube_point(faces[face_index].corners[corner_index], right, up, cube_center, cube_scale);
		}
		if (cube_hovered && point_in_convex_quad(polygon, mouse)) {
			hovered_face_index = static_cast<int>(face_index);
			break;
		}
	}

	for (std::size_t face_index : visible_faces) {
		std::array<ImVec2, 4> polygon{};
		ImVec2 label_position(0.0f, 0.0f);
		for (std::size_t corner_index = 0; corner_index < faces[face_index].corners.size(); ++corner_index) {
			polygon[corner_index] =
				project_view_cube_point(faces[face_index].corners[corner_index], right, up, cube_center, cube_scale);
			label_position.x += polygon[corner_index].x;
			label_position.y += polygon[corner_index].y;
		}
		label_position.x /= static_cast<float>(polygon.size());
		label_position.y /= static_cast<float>(polygon.size());

		ImVec4 fill = color_from_hex(faces[face_index].fill_color, 0.92f);
		if (static_cast<int>(face_index) == hovered_face_index) {
			fill.x = std::min(fill.x + 0.10f, 1.0f);
			fill.y = std::min(fill.y + 0.10f, 1.0f);
			fill.z = std::min(fill.z + 0.10f, 1.0f);
		}
		draw_list->AddConvexPolyFilled(polygon.data(),
		                               static_cast<int>(polygon.size()),
		                               ImGui::GetColorU32(fill));
		draw_list->AddPolyline(polygon.data(),
		                       static_cast<int>(polygon.size()),
		                       ImGui::GetColorU32(color_from_hex(0xfefcf8, 0.98f)),
		                       ImDrawFlags_Closed,
		                       1.5f);
		const ImVec2 label_size = ImGui::CalcTextSize(faces[face_index].label);
		draw_list->AddText(ImVec2(label_position.x - label_size.x * 0.5f,
		                          label_position.y - label_size.y * 0.5f),
		                   ImGui::GetColorU32(color_from_hex(0x2f2d29)),
		                   faces[face_index].label);
	}

	if (hovered_face_index >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
		snap_camera_to_view_cube_face(faces[static_cast<std::size_t>(hovered_face_index)].normal);
		return true;
	}
	return false;
}

void reset_preview_simulation_timing()
{
	preview_simulation_accumulated_time() = 0.0f;
	preview_simulation_interpolation_alpha() = 1.0f;
}

bool is_preview_dynamic_cube_instance(const PrimitiveInstance &instance)
{
	return !instance.removed &&
	       !instance.immovable &&
	       (instance.type == "Cube" ||
	        instance.type == "CubeX" ||
	        instance.type == "CubeY" ||
	        instance.type == "CubeZ");
}

PreviewDynamicCubeMode preview_dynamic_cube_mode_for_instance(const PrimitiveInstance &instance)
{
	if (instance.type == "CubeX") {
		return PreviewDynamicCubeMode::CubeX;
	}
	if (instance.type == "CubeY") {
		return PreviewDynamicCubeMode::CubeY;
	}
	if (instance.type == "CubeZ") {
		return PreviewDynamicCubeMode::CubeZ;
	}
	return PreviewDynamicCubeMode::Default;
}

void upload_static_preview_buffers(EditorWorkspaceSession *app_state, Context *context)
{
	if (context == nullptr) {
		upload_render_buffers(std::vector<std::vector<GLfloat>>{},
		                      std::vector<int>{},
		                      active_materials.size());
		upload_transparent_render_batches(std::vector<PreviewTransparentBatch>{});
		return;
	}

	sync_active_materials(app_state, context->getMaterialNames());
	const auto base_vertices = build_base_vertex_data();
	std::vector<std::vector<GLfloat>> buffers(active_materials.size());
	std::vector<int> counts(active_materials.size(), 0);
	std::vector<PreviewTransparentBatch> transparent_batches;
	transparent_batches.reserve(context->primitive_instances.size());
	for (const PrimitiveInstance &instance : context->primitive_instances) {
		if (instance.removed ||
		    is_preview_dynamic_cube_instance(instance) ||
		    instance.material_index < 0) {
			continue;
		}
		const std::size_t material_index = static_cast<std::size_t>(instance.material_index);
		if (material_index >= active_materials.size()) {
			continue;
		}
		if (active_materials[material_index].opacity < 0.999f) {
			PreviewTransparentBatch batch;
			batch.material_index = instance.material_index;
			batch.center = context->getInstanceCenter(instance);
			context->buildInstanceBuffer(instance, base_vertices.data(), &batch.vertices);
			if (!batch.vertices.empty()) {
				transparent_batches.push_back(std::move(batch));
			}
			continue;
		}

		context->buildInstanceBuffer(instance, base_vertices.data(), &buffers[material_index]);
	}

	for (std::size_t material_index = 0; material_index < buffers.size(); ++material_index) {
		counts[material_index] = static_cast<int>(buffers[material_index].size() / 8u);
	}
	upload_render_buffers(buffers, counts, active_materials.size());
	upload_transparent_render_batches(transparent_batches);
}

void upload_dynamic_preview_buffers(Context *context)
{
	std::vector<PreviewDynamicCubeDraw> draws;
	if (context == nullptr) {
		upload_dynamic_cube_draws(draws);
		return;
	}

	draws.reserve(context->primitive_instances.size());
	for (const PrimitiveInstance &instance : context->primitive_instances) {
		if (!is_preview_dynamic_cube_instance(instance) || instance.material_index < 0) {
			continue;
		}
		const std::size_t material_index = static_cast<std::size_t>(instance.material_index);
		if (material_index >= active_materials.size()) {
			continue;
		}

		PreviewDynamicCubeDraw draw;
		draw.previous_primary_transform =
			instance.has_previous_render_state ? instance.previous_primary_transform : instance.primary_transform;
		draw.previous_secondary_transform =
			instance.has_previous_render_state ? instance.previous_secondary_transform : instance.secondary_transform;
		draw.current_primary_transform = instance.primary_transform;
		draw.current_secondary_transform = instance.secondary_transform;
		draw.previous_center =
			instance.has_previous_render_state ? instance.previous_bounds_center : context->getInstanceCenter(instance);
		draw.current_center = context->getInstanceCenter(instance);
		draw.dual_scales = instance.dual_scales;
		draw.dual_translations = instance.dual_translations;
		draw.texscale = instance.texscale;
		draw.material_index = instance.material_index;
		draw.mode = preview_dynamic_cube_mode_for_instance(instance);
		draws.push_back(std::move(draw));
	}
	upload_dynamic_cube_draws(draws);
}

void upload_context_preview_buffers(EditorWorkspaceSession *app_state, Context *context)
{
	upload_static_preview_buffers(app_state, context);
	upload_dynamic_preview_buffers(context);
}

void step_preview_simulation(float delta_time)
{
	if (grammar == nullptr || grammar->context == nullptr || delta_time <= 0.0f) {
		return;
	}

	preview_simulation_accumulated_time() = std::min(
		preview_simulation_accumulated_time() + delta_time,
		kPreviewSimulationStep * static_cast<float>(kMaxPreviewSimulationStepsPerFrame));
	bool advanced_simulation = false;
	int step_count = 0;
	while (preview_simulation_accumulated_time() >= kPreviewSimulationStep &&
	       step_count < kMaxPreviewSimulationStepsPerFrame) {
		grammar->context->stepSimulation(kPreviewSimulationStep);
		preview_simulation_accumulated_time() -= kPreviewSimulationStep;
		advanced_simulation = true;
		++step_count;
	}
	if (advanced_simulation) {
		upload_dynamic_preview_buffers(grammar->context);
	}
	preview_simulation_interpolation_alpha() =
		std::clamp(preview_simulation_accumulated_time() / kPreviewSimulationStep, 0.0f, 1.0f);
}

void step_preview_simulation_once(float delta_time)
{
	if (grammar == nullptr || grammar->context == nullptr || delta_time <= 0.0f) {
		return;
	}

	reset_preview_simulation_timing();
	grammar->context->stepSimulation(delta_time);
	upload_dynamic_preview_buffers(grammar->context);
	preview_simulation_interpolation_alpha() = 1.0f;
}

void ensure_scene_generation_services()
{
	if (grammar_compilation_service == nullptr) {
		grammar_compilation_service = std::make_unique<GrammarCompilationService>(
			seed_grammar_rng_for_generation,
			[](GrammarDocument &grammar_document, bool quiet) {
				return validate_grammar(&grammar_document, nullptr, quiet);
				});
	}
	if (scene_regeneration_coordinator != nullptr) {
		return;
	}
	GrammarCompilationService *compilation_service = grammar_compilation_service.get();
	scene_regeneration_coordinator =
		std::make_unique<SceneRegenerationCoordinator>(
			[compilation_service](const SceneGenerationRequest &request) {
				return compilation_service->compile(request);
			});
}

void request_scene_regeneration_with_options(const std::string &source_text,
                                             double evaluation_time,
                                             uint64_t design_nonce,
                                             bool time_sample)
{
	ensure_scene_generation_services();
	if (scene_regeneration_coordinator == nullptr) {
		return;
	}

	if (!time_sample) {
		clear_console_messages();
	}
	scene_regeneration_coordinator->requestGeneration(source_text,
	                                                  evaluation_time,
	                                                  design_nonce,
	                                                  time_sample);
}

void request_scene_regeneration_internal(const std::string &source_text)
{
	if (scene_regeneration_coordinator != nullptr) {
		scene_regeneration_coordinator->cancelScheduledRequest();
	}
	preview_grammar_target_time() = 0.0;
	preview_grammar_request_accumulator() = 0.0;
	const uint64_t design_nonce = ++next_grammar_design_nonce;
	request_scene_regeneration_with_options(source_text,
	                                        0.0,
	                                        design_nonce,
	                                        false);
}

void request_scene_time_sample_internal(const std::string &source_text,
                                        double evaluation_time)
{
	if (!std::isfinite(evaluation_time)) {
		errorout("Grammar time sample rejected because t is not finite.");
		return;
	}
	uint64_t design_nonce = active_grammar_design_nonce;
	if (scene_regeneration_coordinator != nullptr &&
	    scene_regeneration_coordinator->pendingDesignNonce() != 0) {
		design_nonce = scene_regeneration_coordinator->pendingDesignNonce();
	} else if (scene_generation_in_progress() &&
	           scene_regeneration_coordinator->inflightDesignNonce() != 0) {
		design_nonce = scene_regeneration_coordinator->inflightDesignNonce();
	}
	if (design_nonce == 0) {
		design_nonce = ++next_grammar_design_nonce;
	}
	request_scene_regeneration_with_options(source_text,
	                                        evaluation_time,
	                                        design_nonce,
	                                        true);
}

// Validate grammar and report any issues
bool validate_grammar(Grammar *gram, EditorWorkspaceSession *app_state, bool quiet)
{
	if (gram == nullptr) {
		if (!quiet) {
			errorout("Grammar validation error: grammar is null.");
		}
		return false;
	}

	if (!quiet) {
		debugout("Validating grammar structure");
	}
	bool valid = true;

	if (app_state != nullptr) {
		app_state->document_diagnostics.clear();
	}

	const auto add_validation_issue = [&](const GrammarDiagnostic &diagnostic) {
		valid = false;
		add_app_state_grammar_diagnostic(app_state, diagnostic);

		if (diagnostic.line >= 0 &&
		    std::find(gram->error_lines.begin(), gram->error_lines.end(), diagnostic.line) ==
		        gram->error_lines.end()) {
			gram->error_lines.push_back(diagnostic.line);
		}
		const bool duplicate = std::any_of(
			gram->error_details.begin(),
			gram->error_details.end(),
			[&](const GrammarDiagnostic &existing) {
				return existing.line == diagnostic.line &&
				       existing.start_column == diagnostic.start_column &&
				       existing.end_column == diagnostic.end_column &&
				       existing.message == diagnostic.message;
			});
		if (!duplicate) {
			gram->error_details.push_back(diagnostic);
		}

		std::ostringstream ss;
		ss << "Grammar validation error";
		if (diagnostic.line >= 0) {
			ss << " [line " << (diagnostic.line + 1);
			if (diagnostic.start_column >= 0) {
				ss << ", column " << (diagnostic.start_column + 1);
			}
			ss << "]";
		}
		ss << ": " << diagnostic.message;
		if (!quiet) {
			errorout(ss.str());
		}
	};

	const auto is_open_delimiter = [](char c) {
		return c == '(' || c == '[' || c == '{';
	};

	const auto is_close_delimiter = [](char c) {
		return c == ')' || c == ']' || c == '}';
	};

	const auto matching_delimiter = [](char c) {
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
	};

	const auto delimiters_match = [&](char open, char close) {
		return matching_delimiter(open) == close;
	};

	const std::vector<std::string> &source_lines = gram->lines;
	if (source_lines.empty()) {
		GrammarDiagnostic diagnostic;
		diagnostic.message = "Document is empty.";
		add_validation_issue(diagnostic);
		return false;
	}

	if (gram->rule_list.empty()) {
		GrammarDiagnostic diagnostic;
		diagnostic.message = "No valid rules were parsed.";
		add_validation_issue(diagnostic);
	} else if (!quiet) {
		debugout("Parsed rules: " + std::to_string(gram->rule_list.size()));
	}

	int empty_rules = 0;
	for (const auto& rule : gram->rule_list) {
		if (rule->tokens.empty()) {
			empty_rules++;
			if (!quiet) {
				errorout("Grammar validation warning: rule '" + rule->rule_name + "' has no tokens in its main section.");
			}
		}
	}

	if (empty_rules > 0 && !quiet) {
		debugout("Rules with empty main sections: " + std::to_string(empty_rules));
	}

	struct DelimiterState {
		char delimiter = '\0';
		int line = -1;
		int column = 0;
	};

		std::vector<DelimiterState> delimiter_stack;
	for (int line_index = 0; line_index < static_cast<int>(source_lines.size()); ++line_index) {
		const std::string &line = source_lines[static_cast<std::size_t>(line_index)];
		const std::size_t comment_column = find_comment_start(line);
		const std::size_t code_end = comment_column == std::string::npos ? line.size() : comment_column;
		for (std::size_t column = 0; column < code_end; ++column) {
			const char c = line[column];
			if (c == '\\') {
				add_validation_issue(
					{line_index,
					 static_cast<int>(column),
					 static_cast<int>(column) + 1,
					 "Unexpected '\\' character in grammar source."});
				continue;
			}
			if (is_open_delimiter(c)) {
				delimiter_stack.push_back({c, line_index, static_cast<int>(column)});
					continue;
			}
			if (!is_close_delimiter(c)) {
				continue;
			}
			if (delimiter_stack.empty()) {
				add_validation_issue(
					{line_index,
					 static_cast<int>(column),
					 static_cast<int>(column) + 1,
					 std::string("Unmatched closing delimiter '") + c + "'."});
				continue;
			}
			const DelimiterState open = delimiter_stack.back();
			if (delimiters_match(open.delimiter, c)) {
				delimiter_stack.pop_back();
				continue;
			}
			add_validation_issue(
				{line_index,
				 static_cast<int>(column),
				 static_cast<int>(column) + 1,
				 std::string("Mismatched closing delimiter '") + c +
				     "'. Expected '" + matching_delimiter(open.delimiter) + "'."});
		}
	}

	for (const DelimiterState &open : delimiter_stack) {
		add_validation_issue(
			{open.line,
			 open.column,
			 open.column + 1,
			 std::string("Unclosed delimiter '") + open.delimiter + "'."});
	}

	const std::vector<ValidationRuleBlock> rule_blocks =
		build_validation_rule_blocks(source_lines);
	for (const ValidationRuleBlock &block : rule_blocks) {
		std::vector<std::size_t> arrow_indices;
		for (std::size_t token_index = 0; token_index < block.tokens.size(); ++token_index) {
			if (block.tokens[token_index].text == "->") {
				arrow_indices.push_back(token_index);
			}
		}
		if (arrow_indices.empty()) {
			add_validation_issue(
				make_line_grammar_diagnostic(source_lines,
				                             block.start_line,
				                             "Rule is missing a '->' production arrow."));
			continue;
		}
		if (arrow_indices[0] == 0) {
			add_validation_issue(
				make_token_grammar_diagnostic(block.tokens[arrow_indices[0]],
				                              "Production arrow needs a rule name on the left-hand side."));
		}
		if (arrow_indices.size() > 2) {
			add_validation_issue(
				make_token_grammar_diagnostic(block.tokens[arrow_indices[2]],
				                              "Rule has too many '->' clauses. Only one alternate production is supported."));
		}

		const std::size_t primary_body_end =
			arrow_indices.size() > 1 ? arrow_indices[1] : block.tokens.size();
		if (arrow_indices[0] + 1 >= primary_body_end) {
			add_validation_issue(
				make_token_grammar_diagnostic(block.tokens[arrow_indices[0]],
				                              "Rule production body is empty."));
		}

		if (arrow_indices.size() > 1 && arrow_indices[1] + 1 >= block.tokens.size()) {
			add_validation_issue(
				make_token_grammar_diagnostic(block.tokens[arrow_indices[1]],
				                              "Alternate production body is empty."));
		}

		std::size_t section_start = arrow_indices[0] + 1;
		std::size_t pipe_count = 0;
		for (std::size_t token_index = arrow_indices[0] + 1;
		     token_index < primary_body_end;
		     ++token_index) {
			if (block.tokens[token_index].text != "|") {
				continue;
			}
			++pipe_count;
			if (section_start == token_index) {
				add_validation_issue(
					make_token_grammar_diagnostic(block.tokens[token_index],
					                              "Rule has an empty section around '|'."));
			}
			if (pipe_count > 2) {
				add_validation_issue(
					make_token_grammar_diagnostic(block.tokens[token_index],
					                              "Rule has too many '|' sections. At most three are supported."));
			}
			section_start = token_index + 1;
		}
		if (section_start == primary_body_end &&
		    primary_body_end > arrow_indices[0] + 1 &&
		    block.tokens[primary_body_end - 1].text == "|") {
			add_validation_issue(
				make_token_grammar_diagnostic(block.tokens[primary_body_end - 1],
				                              "Rule has an empty section around '|'."));
		}
	}

	if (!quiet) {
		if (app_state != nullptr && !app_state->document_diagnostics.empty()) {
			debugout("Grammar validation found " +
			         std::to_string(app_state->document_diagnostics.size()) + " issue(s)");
		} else {
			debugout("Grammar validation completed without additional structural issues");
		}
	}

	return valid;
}

void process_completed_regeneration(EditorWorkspaceSession *app_state)
{
	if (scene_regeneration_coordinator == nullptr) {
		return;
	}

	std::unique_ptr<SceneGenerationResult> result =
		scene_regeneration_coordinator->takeCompletedResult();
	if (result == nullptr) {
		return;
	}

	if (!scene_regeneration_coordinator->isLatestRequest(result->request_id)) {
		debugout("Discarded stale regeneration result");
	} else if (!result->succeeded()) {
		if (result->time_sample) {
			preview_playback_active() = false;
		}
		if (app_state != nullptr) {
			app_state->document_diagnostics.clear();
			for (int diagnostic_line : result->diagnostic_lines) {
				app_state->document_diagnostics.addLine(diagnostic_line);
			}
			for (const GrammarDiagnostic &diagnostic : result->diagnostics) {
				add_app_state_grammar_diagnostic(app_state, diagnostic);
			}
		}
		if (!result->error.empty()) {
			errorout(result->error);
		} else if (result->time_sample) {
			errorout("Grammar time sample failed; retaining the last valid scene snapshot.");
		} else {
			errorout("Scene generation failed; retaining the last valid scene snapshot.");
		}
	} else {
		std::unique_ptr<GeneratedSceneSnapshot> candidate_snapshot =
			std::move(result->scene_snapshot);
		bool preview_upload_succeeded = true;
		try {
			upload_context_preview_buffers(app_state,
			                               candidate_snapshot->generationContext());
		}
		catch (const std::exception &exception) {
			errorout(std::string("Preview upload failed: ") + exception.what());
			preview_upload_succeeded = false;
		}
		catch (...) {
			errorout("Preview upload failed");
			preview_upload_succeeded = false;
		}
		if (preview_upload_succeeded) {
			ElectricalControlGraph previous_control_graph =
				current_preview_session().lighting.electrical_control_graph;
			destroy_grammar();
			current_scene_snapshot() = std::move(candidate_snapshot);
			grammar = current_scene_snapshot()->grammarDocument();
			active_grammar_design_nonce = current_scene_snapshot()->designNonce();
			if (app_state != nullptr) {
				app_state->document.recordSuccessfulGeneration(
					result->request_id,
					current_scene_snapshot()->designNonce());
			}
			current_preview_session().source_associations =
				SceneSourceAssociationIndex::fromContext(
					*current_scene_snapshot()->generationContext());
			const LightingSceneDefinition &lighting_definition =
				current_scene_snapshot()->generationContext()->lightingSceneDefinition();
			current_preview_session().lighting.lights.replaceGrammarLights(
				lighting_definition.lights());
			current_preview_session().lighting.electrical_control_graph =
				lighting_definition.electricalControlGraph();
			for (LightSwitch &light_switch :
			     current_preview_session().lighting.electrical_control_graph.switches()) {
				const LightSwitch *previous_switch = previous_control_graph.findSwitch(light_switch.id());
				if (previous_switch != nullptr) light_switch.state() = previous_switch->state();
			}
			LightingStateEvaluator().apply(
				current_preview_session().lighting.electrical_control_graph,
				&current_preview_session().lighting.lights);
			current_preview_session().statistics.generation_milliseconds =
				result->generation_milliseconds;
			current_preview_session().statistics.rule_count = grammar->rule_list.size();
			current_preview_session().statistics.token_count = grammar->tokens_new.size();
			current_preview_session().statistics.primitive_count =
				current_scene_snapshot()->generationContext()->primitive_instances.size();
			current_preview_session().statistics.material_count =
				current_scene_snapshot()->generationContext()->getMaterialNames().size();
			if (!result->time_sample) {
				preview_playback_active() = false;
				preview_grammar_target_time() = current_scene_snapshot()->evaluationTime();
				preview_grammar_request_accumulator() = 0.0;
			}
			reset_preview_simulation_timing();
			if (!result->time_sample) {
				debugout("Regenerated scene");
			}
			if (app_state != nullptr && !result->time_sample) {
				scene_selection_controller.clear(current_preview_session().selection);
				app_state->document_diagnostics.clear();
			}
		}
	}

	scene_regeneration_coordinator->startPendingRequest();

	if (smart_editor_state.preserve_focus_through_regeneration) {
		request_editor_focus_restore();
		if (!scene_generation_queued() && !scene_generation_in_progress()) {
			smart_editor_state.preserve_focus_through_regeneration = false;
		}
	}
}

void set_editor_document_internal(EditorWorkspaceSession *app_state,
                                  const std::string &text,
                                  bool dirty)
{
	if (app_state == nullptr) {
		return;
	}

	app_state->document.replaceSourceText(text, dirty);
	if (scene_regeneration_coordinator != nullptr) {
		scene_regeneration_coordinator->cancelScheduledRequest();
	}
	current_preview_session().fit_camera_pending = true;
	app_state->document.clearLocalIdentity();
	app_state->local_document_workflow.reset();
	clear_cloud_document_identity(app_state);
	smart_editor_state.preserve_focus_through_regeneration = false;
	reset_editor_interaction_state();
}


BackendTextureSlot *find_texture_library_entry(TextureLibraryState *state, const std::string &slot)
{
	if (state == nullptr) {
		return nullptr;
	}
	for (BackendTextureSlot &entry : state->entries) {
		if (entry.slot == slot) {
			return &entry;
		}
	}
	return nullptr;
}

BackendTextureSlot *selected_texture_library_entry(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return nullptr;
	}
	return find_texture_library_entry(&app_state->texture_library, app_state->texture_library.selected_slot);
}

const BackendTextureSlot *selected_texture_library_entry(const EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return nullptr;
	}
	return find_texture_library_entry(const_cast<TextureLibraryState *>(&app_state->texture_library),
	                                  app_state->texture_library.selected_slot);
}

void destroy_texture_library_preview(TextureLibraryState *state)
{
	if (state == nullptr) {
		return;
	}
	if (state->preview_texture != 0) {
		glDeleteTextures(1, &state->preview_texture);
		state->preview_texture = 0;
	}
	state->preview_width = 0;
	state->preview_height = 0;
	state->preview_slot.clear();
	state->preview_updated_at.clear();
}

void clear_texture_library_state_internal(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return;
	}
	destroy_texture_library_preview(&app_state->texture_library);
	app_state->texture_library = TextureLibraryState{};
}

void set_texture_library_feedback(EditorWorkspaceSession *app_state, const std::string &message, bool is_error)
{
	if (app_state == nullptr) {
		return;
	}
	app_state->texture_library.status_message = message;
	app_state->texture_library.status_is_error = is_error;
}

std::vector<BackendTextureSlot> normalize_texture_library_entries(const std::vector<BackendTextureSlot> &entries)
{
	std::unordered_map<std::string, BackendTextureSlot> by_slot;
	for (const BackendTextureSlot &entry : entries) {
		by_slot[entry.slot] = entry;
	}

	std::vector<BackendTextureSlot> normalized;
	normalized.reserve(kBackendTextureSlotCount);
	for (int index = 1; index <= static_cast<int>(kBackendTextureSlotCount); ++index) {
		const std::string slot = "usertexture" + std::to_string(index);
		auto it = by_slot.find(slot);
		normalized.push_back(it != by_slot.end() ? it->second : make_texture_slot_placeholder(index));
	}
	return normalized;
}

bool load_selected_texture_preview(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return false;
	}

	TextureLibraryState &state = app_state->texture_library;
	BackendTextureSlot *entry = selected_texture_library_entry(app_state);
	if (entry == nullptr || !entry->active) {
		destroy_texture_library_preview(&state);
		return true;
	}
	if (state.preview_texture != 0 &&
	    state.preview_slot == entry->slot &&
	    state.preview_updated_at == entry->updated_at) {
		return true;
	}

	std::string image_bytes;
	std::string error;
	BackendTextureLibraryRepository texture_repository(app_state->authentication.config,
	                                                   app_state->authentication.session);
	if (!texture_repository.fetchPreviewImage(entry->slot, &image_bytes, &error)) {
		destroy_texture_library_preview(&state);
		set_texture_library_feedback(app_state, error.empty() ? "Could not load the texture preview." : error, true);
		return false;
	}

	int width = 0;
	int height = 0;
	std::vector<unsigned char> rgba_pixels;
	if (!decode_texture_image_bytes(image_bytes, &rgba_pixels, &width, &height) || rgba_pixels.empty()) {
		destroy_texture_library_preview(&state);
		set_texture_library_feedback(app_state, "Could not decode the texture preview image.", true);
		return false;
	}

	const GLuint texture = upload_rgba_texture(rgba_pixels.data(), width, height);
	if (texture == 0) {
		destroy_texture_library_preview(&state);
		set_texture_library_feedback(app_state, "OpenGL could not upload the texture preview.", true);
		return false;
	}

	destroy_texture_library_preview(&state);
	state.preview_texture = texture;
	state.preview_width = width;
	state.preview_height = height;
	state.preview_slot = entry->slot;
	state.preview_updated_at = entry->updated_at;
	return true;
}

void sync_texture_library_inputs_from_selection(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return;
	}

	TextureLibraryState &state = app_state->texture_library;
	BackendTextureSlot *entry = selected_texture_library_entry(app_state);
	if (entry == nullptr) {
		state.selected_slot = "usertexture1";
		entry = selected_texture_library_entry(app_state);
	}
	if (entry == nullptr) {
		destroy_texture_library_preview(&state);
		state.display_name_input = state.selected_slot;
		state.prompt_input.clear();
		state.alpha_value = 1.0f;
		return;
	}

	state.display_name_input = entry->display_name.empty() ? entry->slot : entry->display_name;
	state.prompt_input = entry->prompt;
	state.alpha_value = std::clamp(entry->alpha, 0.0f, 1.0f);
	if (!entry->active) {
		destroy_texture_library_preview(&state);
		return;
	}
	load_selected_texture_preview(app_state);
}

void invalidate_texture_materials_internal(EditorWorkspaceSession *app_state)
{
	destroy_materials();
	if (app_state == nullptr) {
		return;
	}
	if (grammar != nullptr || !trim_copy(app_state->document.source_text).empty()) {
		request_scene_regeneration_internal(app_state->document.source_text);
	}
}

void apply_texture_library_entries(EditorWorkspaceSession *app_state,
                                   const std::vector<BackendTextureSlot> &entries,
                                   bool preserve_status_message)
{
	if (app_state == nullptr) {
		return;
	}

	TextureLibraryState &state = app_state->texture_library;
	const std::string previous_selected_slot = state.selected_slot;
	state.entries = normalize_texture_library_entries(entries);
	if (find_texture_library_entry(&state, previous_selected_slot) != nullptr) {
		state.selected_slot = previous_selected_slot;
	} else {
		state.selected_slot = "usertexture1";
	}
	sync_texture_library_inputs_from_selection(app_state);
	if (!preserve_status_message) {
		set_texture_library_feedback(app_state, "", false);
	}
	invalidate_texture_materials_internal(app_state);
}

void refresh_texture_library_internal(EditorWorkspaceSession *app_state, bool preserve_status_message)
{
	if (app_state == nullptr) {
		return;
	}
	if (!app_state->authentication.session.authenticated || trim_copy(app_state->authentication.config.backend_base_url).empty()) {
		clear_texture_library_state_internal(app_state);
		return;
	}

	std::vector<BackendTextureSlot> entries;
	std::string error;
	BackendTextureLibraryRepository texture_repository(app_state->authentication.config,
	                                                   app_state->authentication.session);
	if (!texture_repository.listSlots(&entries, &error)) {
		set_texture_library_feedback(app_state,
		                             error.empty() ? "Could not load the texture library." : error,
		                             true);
		return;
	}

	apply_texture_library_entries(app_state, entries, preserve_status_message);
}

void set_ai_assistant_feedback(EditorWorkspaceSession *app_state, const std::string &message, bool is_error)
{
	if (app_state == nullptr) {
		return;
	}
	app_state->ai_assistant.status_message = message;
	app_state->ai_assistant.status_is_error = is_error;
}

std::string selected_editor_text(const EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr || app_state->document.source_text.empty()) {
		return "";
	}

	const int selection_start =
		std::clamp(std::min(smart_editor_state.selection_start, smart_editor_state.selection_end),
		           0,
		           static_cast<int>(app_state->document.source_text.size()));
	const int selection_end =
		std::clamp(std::max(smart_editor_state.selection_start, smart_editor_state.selection_end),
		           0,
		           static_cast<int>(app_state->document.source_text.size()));
	if (selection_start >= selection_end) {
		return "";
	}
	return app_state->document.source_text.substr(static_cast<std::size_t>(selection_start),
	                                     static_cast<std::size_t>(selection_end - selection_start));
}

std::string current_parser_error_summary(const EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr || app_state->document_diagnostics.empty()) {
		return "";
	}

	std::ostringstream summary;
	const std::vector<GrammarDiagnostic> &diagnostics =
		app_state->document_diagnostics.diagnostics();
	const std::size_t limit = std::min<std::size_t>(diagnostics.size(), 3u);
	for (std::size_t index = 0; index < limit; ++index) {
		const GrammarDiagnostic &diagnostic = diagnostics[index];
		if (index > 0) {
			summary << "\n";
		}
		summary << "Line " << (diagnostic.line + 1);
		if (diagnostic.start_column >= 0) {
			summary << ", Col " << (diagnostic.start_column + 1);
		}
		summary << ": " << trim_copy(diagnostic.message);
	}
	return summary.str();
}

std::vector<std::string> ai_request_history(const AiAssistantSession &state)
{
	std::vector<std::string> history;
	const std::size_t message_count = state.messages.size();
	const std::size_t history_start = message_count > 6 ? message_count - 6 : 0;
	for (std::size_t index = history_start; index < message_count; ++index) {
		const BackendAiMessage &message = state.messages[index];
		if (trim_copy(message.content).empty()) {
			continue;
		}
		const std::string prefix = lowercase_copy(message.role) == "assistant" ? "Assistant: " : "User: ";
		history.push_back(prefix + trim_copy(message.content));
	}
	return history;
}

void sync_ai_result_from_messages(AiAssistantSession *state)
{
	if (state == nullptr) {
		return;
	}

	state->last_result = BackendAiResult{};
	for (auto it = state->messages.rbegin(); it != state->messages.rend(); ++it) {
		if (lowercase_copy(it->role) == "assistant" && it->result.loaded) {
			state->last_result = it->result;
			break;
		}
	}
}

void stage_ai_proposal_review(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return;
	}
	AiAssistantSession &state = app_state->ai_assistant;
	if (!state.last_result.loaded || trim_copy(state.last_result.grammar).empty()) {
		state.proposal_review.clear();
		return;
	}
	state.proposal_review.beginReview(app_state->document.source_text,
	                                  state.last_result.grammar,
	                                  state.last_result.warnings);
}

void apply_ai_thread_payload(EditorWorkspaceSession *app_state,
                             const BackendAiThreadSummary &thread,
                             const std::vector<BackendAiMessage> &messages,
                             bool preserve_status_message)
{
	if (app_state == nullptr) {
		return;
	}

	AiAssistantSession &state = app_state->ai_assistant;
	state.current_thread = thread;
	state.selected_thread_id = thread.id;
	state.messages = messages;
	state.mode = !thread.mode.empty() ? thread.mode : state.mode;
	state.last_usage = BackendAiUsageSummary{};
	state.active_model.clear();
	state.threads_loaded = true;
	sync_ai_result_from_messages(&state);
	stage_ai_proposal_review(app_state);
	if (!preserve_status_message) {
		set_ai_assistant_feedback(app_state, "", false);
	}
}

bool load_ai_thread(EditorWorkspaceSession *app_state,
                    const std::string &thread_id,
                    bool preserve_status_message = false)
{
	if (app_state == nullptr) {
		return false;
	}

	const std::string normalized_thread_id = trim_copy(thread_id);
	if (normalized_thread_id.empty()) {
		set_ai_assistant_feedback(app_state, "AI thread id is empty.", true);
		return false;
	}

	BackendAiThreadSummary thread;
	std::vector<BackendAiMessage> messages;
	std::string error;
	BackendAiGrammarProposalService proposal_service(app_state->authentication.config,
	                                                  app_state->authentication.session);
	if (!proposal_service.loadThread(normalized_thread_id,
	                                &thread,
	                                &messages,
	                                &error)) {
		set_ai_assistant_feedback(app_state,
		                          error.empty() ? "Could not load the AI thread." : error,
		                          true);
		return false;
	}

	persist_current_firebase_session(app_state);
	apply_ai_thread_payload(app_state, thread, messages, preserve_status_message);
	return true;
}

void refresh_ai_threads_internal(EditorWorkspaceSession *app_state, bool preserve_status_message)
{
	if (app_state == nullptr) {
		return;
	}
	if (!app_state->authentication.session.authenticated || trim_copy(app_state->authentication.config.backend_base_url).empty()) {
		clear_ai_assistant_state_internal(app_state);
		return;
	}

	std::vector<BackendAiThreadSummary> threads;
	std::string error;
	BackendAiGrammarProposalService proposal_service(app_state->authentication.config,
	                                                  app_state->authentication.session);
	if (!proposal_service.listThreads(&threads, &error)) {
		set_ai_assistant_feedback(app_state,
		                          error.empty() ? "Could not load AI threads." : error,
		                          true);
		return;
	}

	persist_current_firebase_session(app_state);
	AiAssistantSession &state = app_state->ai_assistant;
	const std::string selected_thread_id = state.selected_thread_id;
	state.threads = std::move(threads);
	state.threads_loaded = true;
	if (!selected_thread_id.empty()) {
		auto selected_it = std::find_if(state.threads.begin(),
		                                state.threads.end(),
		                                [&](const BackendAiThreadSummary &thread) {
			                                return thread.id == selected_thread_id;
		                                });
		if (selected_it != state.threads.end()) {
			state.current_thread = *selected_it;
		} else {
			state.current_thread = BackendAiThreadSummary{};
			state.selected_thread_id.clear();
			state.messages.clear();
			state.last_usage = BackendAiUsageSummary{};
			state.last_result = BackendAiResult{};
			state.proposal_review.clear();
			state.active_model.clear();
		}
	}
	if (!preserve_status_message) {
		set_ai_assistant_feedback(app_state, "", false);
	}
}

void start_new_ai_thread(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return;
	}

	AiAssistantSession &state = app_state->ai_assistant;
	state.current_thread = BackendAiThreadSummary{};
	state.selected_thread_id.clear();
	state.messages.clear();
	state.last_usage = BackendAiUsageSummary{};
	state.last_result = BackendAiResult{};
	state.proposal_review.clear();
	state.active_model.clear();
	set_ai_assistant_feedback(app_state, "Started a new AI thread.", false);
}

const char *cloud_dialog_title(CloudDialogAction action)
{
	switch (action) {
	case CloudDialogAction::Open:
		return "Open Cloud File";
	case CloudDialogAction::SaveAs:
		return "Save File To Cloud";
	case CloudDialogAction::None:
	default:
		return "Cloud File";
	}
}

const char *cloud_dialog_confirm_label(CloudDialogAction action)
{
	switch (action) {
	case CloudDialogAction::Open:
		return "Open";
	case CloudDialogAction::SaveAs:
		return "Save";
	case CloudDialogAction::None:
	default:
		return "Confirm";
	}
}

void open_cloud_dialog(EditorWorkspaceSession *app_state, CloudDialogAction action)
{
	if (app_state == nullptr) {
		return;
	}

	cancel_cloud_dialog_async_requests(app_state);

	CloudDialogState &state = app_state->cloud_dialog;
	state.action = action;
	state.request_open = true;
	state.request_focus = true;
	state.selected_file_id = app_state->storage_identity.cloud_file_id;
	state.name_input = default_cloud_title(app_state);
	request_cloud_dialog_refresh(app_state, "Loading cloud files...");
}

void draw_cloud_document_dialog_content(EditorWorkspaceSession &workspace_session)
{
	EditorWorkspaceSession *app_state = &workspace_session;

	CloudDialogState &state = app_state->cloud_dialog;
	if (state.action == CloudDialogAction::None) {
		return;
	}

	const char *title = cloud_dialog_title(state.action);
	if (state.request_open) {
		ImGui::OpenPopup(title);
		state.request_open = false;
	}

	if (!ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		return;
	}

	ImGui::TextUnformatted("Webhost Backend");
	ImGui::SameLine();
	draw_status_chip("30,000 byte limit", 0xc8a766, 0x7a6035, 0.14f, 0.26f);

	if (state.action == CloudDialogAction::SaveAs) {
		ImGui::Spacing();
		ImGui::TextUnformatted("Title");
		if (state.request_focus) {
			ImGui::SetKeyboardFocusHere();
			state.request_focus = false;
		}
		input_text_string("##cloud_file_title", &state.name_input);
	} else {
		state.request_focus = false;
	}

	ImGui::Spacing();
	ImGui::BeginChild("##cloud_file_entries", ImVec2(560.0f, 280.0f), true);
	for (const BackendFileSummary &entry : state.entries) {
		const bool selected = state.selected_file_id == entry.id;
		if (ImGui::Selectable(entry.title.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
			state.selected_file_id = entry.id;
			state.name_input = entry.title;
			if (state.action == CloudDialogAction::Open &&
			    ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				request_cloud_document_open(app_state, entry.id, entry.is_published);
			}
		}
		if (!entry.updated_at.empty()) {
			ImGui::SameLine(300.0f);
			ImGui::TextUnformatted(entry.updated_at.c_str());
		}
		if (entry.is_published) {
			ImGui::SameLine();
			draw_status_chip("Published", 0x7ea88e, 0x527564, 0.12f, 0.24f);
		}
	}
	ImGui::EndChild();

	bool close_dialog = apply_cloud_dialog_async_results(app_state);
	const bool has_pending_action = state.loading_entries || state.opening_file;
	const bool can_confirm = state.action == CloudDialogAction::Open
	                             ? !trim_copy(state.selected_file_id).empty()
	                             : !trim_copy(state.name_input).empty();
	ImGui::BeginDisabled(!can_confirm || has_pending_action);
	if (ImGui::Button(cloud_dialog_confirm_label(state.action))) {
		if (state.action == CloudDialogAction::Open) {
			bool selected_is_published = false;
			for (const BackendFileSummary &entry : state.entries) {
				if (entry.id == state.selected_file_id) {
					selected_is_published = entry.is_published;
					break;
				}
			}
			request_cloud_document_open(app_state,
			                           state.selected_file_id,
			                           selected_is_published);
		} else if (state.action == CloudDialogAction::SaveAs) {
			close_dialog = save_document_to_cloud(app_state, state.name_input, true);
		}
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(has_pending_action);
	if (ImGui::Button("Refresh")) {
		request_cloud_dialog_refresh(app_state, "Refreshing cloud files...");
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Cancel")) {
		close_dialog = true;
		cancel_cloud_dialog_async_requests(app_state);
	}

	if (!state.status_message.empty()) {
		ImGui::Spacing();
		ImGui::PushStyleColor(ImGuiCol_Text,
		                      state.status_is_error ? color_from_hex(0xb24b47)
		                                            : color_from_hex(0x4d6d56));
		ImGui::TextWrapped("%s", state.status_message.c_str());
		ImGui::PopStyleColor();
	}

	if (close_dialog) {
		cancel_cloud_dialog_async_requests(app_state);
		state.action = CloudDialogAction::None;
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}

enum class DocumentSaveAttempt {
	Saved,
	DialogRequested,
	Failed
};

void set_local_document_feedback(EditorWorkspaceSession *app_state,
	                             const std::string &message,
	                             bool is_error)
{
	if (app_state == nullptr) {
		return;
	}
	app_state->local_document_workflow.status_message = message;
	app_state->local_document_workflow.status_is_error = is_error;
	if (is_error) {
		errorout(message);
	} else if (!message.empty()) {
		debugout(message);
	}
}

std::vector<LocalFileDialogFilter> grammar_file_dialog_filters()
{
	return {
		LocalFileDialogFilter("Progen3D grammar", "p3d,grammar"),
		LocalFileDialogFilter("Text document", "txt")};
}

std::filesystem::path preferred_local_document_directory(
	const EditorWorkspaceSession &app_state)
{
	if (!app_state.document.local_path.empty() &&
	    !app_state.document.local_path.parent_path().empty()) {
		return app_state.document.local_path.parent_path();
	}
	if (!app_state.local_document_workflow.last_successful_directory.empty()) {
		return app_state.local_document_workflow.last_successful_directory;
	}

	const std::filesystem::path examples_directory =
		progen3d_resource_path(std::filesystem::path("examples") / "procedural_primitives");
	std::error_code filesystem_error;
	if (std::filesystem::is_directory(examples_directory, filesystem_error)) {
		return examples_directory;
	}
	const std::filesystem::path general_examples_directory =
		progen3d_resource_path("examples");
	filesystem_error.clear();
	if (std::filesystem::is_directory(general_examples_directory, filesystem_error)) {
		return general_examples_directory;
	}

	const std::filesystem::path current_directory =
		std::filesystem::current_path(filesystem_error);
	return filesystem_error ? std::filesystem::path{} : current_directory;
}

std::string suggested_local_document_filename(const GrammarSourceDocument &document)
{
	std::filesystem::path suggested_name = document.local_path.filename();
	if (suggested_name.empty()) {
		suggested_name = document.title.empty() ? "untitled.p3d" : document.title;
	}
	if (!suggested_name.has_extension()) {
		suggested_name.replace_extension(".p3d");
	}
	return suggested_name.filename().string();
}

void request_local_file_dialog(EditorWorkspaceSession *app_state,
	                           LocalFileDialogPurpose purpose)
{
	if (app_state == nullptr) {
		return;
	}
	if (running_local_file_dialog_service == nullptr) {
		set_local_document_feedback(
			app_state,
			"Native local file dialogs are unavailable in this runtime.",
			true);
		if (purpose == LocalFileDialogPurpose::SaveBeforeDocumentReplacement) {
			app_state->document_replacement_workflow.cancelReplacement();
		}
		return;
	}

	LocalFileDialogRequest request;
	request.purpose = purpose;
	request.initial_directory = preferred_local_document_directory(*app_state);
	request.suggested_filename = suggested_local_document_filename(app_state->document);
	request.filters = grammar_file_dialog_filters();
	app_state->local_document_workflow.requestDialog(std::move(request));
}

std::filesystem::path normalized_local_save_path(std::filesystem::path path)
{
	if (!path.has_extension()) {
		path.replace_extension(".p3d");
	}
	return path.lexically_normal();
}

bool save_document_to_local_path(EditorWorkspaceSession *app_state,
	                             const std::filesystem::path &requested_path)
{
	if (app_state == nullptr) {
		return false;
	}
	static const DocumentPersistenceService document_persistence_service;
	static const LightingScenePersistenceService lighting_persistence_service;
	const std::filesystem::path local_path = normalized_local_save_path(requested_path);
	std::string error_message;
	if (!document_persistence_service.saveGrammarSource(local_path,
	                                                    app_state->document.source_text,
	                                                    &error_message)) {
		set_local_document_feedback(app_state, error_message, true);
		return false;
	}
	if (!lighting_persistence_service.save(local_path,
	                                      app_state->preview.lighting,
	                                      &error_message)) {
		set_local_document_feedback(app_state, error_message, true);
		return false;
	}

	app_state->document.adoptLocalIdentity(local_path);
	app_state->document.markPersisted();
	app_state->local_document_workflow.last_successful_directory = local_path.parent_path();
	set_local_document_feedback(app_state, "Saved " + local_path.string(), false);
	return true;
}

bool open_document_from_local_path(EditorWorkspaceSession *app_state,
	                               const std::filesystem::path &local_path)
{
	if (app_state == nullptr) {
		return false;
	}
	static const DocumentPersistenceService document_persistence_service;
	static const LightingScenePersistenceService lighting_persistence_service;
	std::string source_text;
	std::string error_message;
	if (!document_persistence_service.loadGrammarSource(local_path,
	                                                    &source_text,
	                                                    &error_message)) {
		set_local_document_feedback(app_state, error_message, true);
		return false;
	}
	if (!lighting_persistence_service.load(local_path,
	                                      &app_state->preview.lighting,
	                                      &error_message)) {
		set_local_document_feedback(app_state, error_message, true);
		return false;
	}

	set_editor_document_internal(app_state, source_text, false);
	app_state->document.adoptLocalIdentity(local_path);
	app_state->local_document_workflow.last_successful_directory = local_path.parent_path();
	set_local_document_feedback(app_state, "Opened " + local_path.string(), false);
	request_scene_regeneration_internal(app_state->document.source_text);
	return true;
}

DocumentSaveAttempt save_current_document(EditorWorkspaceSession *app_state,
	                                      bool before_document_replacement)
{
	if (app_state == nullptr) {
		return DocumentSaveAttempt::Failed;
	}
	if (!app_state->document.local_path.empty()) {
		return save_document_to_local_path(app_state, app_state->document.local_path)
			? DocumentSaveAttempt::Saved
			: DocumentSaveAttempt::Failed;
	}
	if (!app_state->storage_identity.cloud_file_id.empty()) {
		return save_document_to_cloud(app_state, default_cloud_title(app_state), false)
			? DocumentSaveAttempt::Saved
			: DocumentSaveAttempt::Failed;
	}

	request_local_file_dialog(
		app_state,
		before_document_replacement
			? LocalFileDialogPurpose::SaveBeforeDocumentReplacement
			: LocalFileDialogPurpose::SaveGrammarAs);
	return app_state->local_document_workflow.pending_dialog_request.has_value()
		? DocumentSaveAttempt::DialogRequested
		: DocumentSaveAttempt::Failed;
}

void perform_document_replacement_action(EditorWorkspaceSession *app_state,
	                                     DocumentReplacementAction action)
{
	if (app_state == nullptr) {
		return;
	}
	switch (action) {
	case DocumentReplacementAction::CreateNewDocument:
		create_new_document(app_state);
		break;
	case DocumentReplacementAction::OpenLocalDocument:
		request_local_file_dialog(app_state, LocalFileDialogPurpose::OpenGrammar);
		break;
	case DocumentReplacementAction::OpenCloudDocument:
		open_cloud_dialog(app_state, CloudDialogAction::Open);
		break;
	case DocumentReplacementAction::ExitApplication:
		application_exit_authorized = true;
		if (running_application_window != nullptr) {
			glfwSetWindowShouldClose(running_application_window, GLFW_TRUE);
		}
		break;
	case DocumentReplacementAction::None:
		break;
	}
}

void request_document_replacement(EditorWorkspaceSession *app_state,
	                              DocumentReplacementAction action)
{
	if (app_state == nullptr) {
		return;
	}
	if (app_state->document_replacement_workflow.request(action,
	                                                    app_state->document.dirty)) {
		perform_document_replacement_action(
			app_state,
			app_state->document_replacement_workflow.authorizeReplacement());
	}
}

const char *document_replacement_description(DocumentReplacementAction action)
{
	switch (action) {
	case DocumentReplacementAction::CreateNewDocument:
		return "create a new grammar";
	case DocumentReplacementAction::OpenLocalDocument:
		return "open another local grammar";
	case DocumentReplacementAction::OpenCloudDocument:
		return "open another cloud grammar";
	case DocumentReplacementAction::ExitApplication:
		return "exit Progen3D";
	case DocumentReplacementAction::None:
	default:
		return "continue";
	}
}

void draw_unsaved_changes_dialog(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return;
	}
	DocumentReplacementWorkflow &workflow = app_state->document_replacement_workflow;
	if (workflow.consumeConfirmationRequest()) {
		ImGui::OpenPopup("Unsaved Changes");
	}
	if (!ImGui::BeginPopupModal("Unsaved Changes", nullptr,
	                            ImGuiWindowFlags_AlwaysAutoResize)) {
		return;
	}

	ImGui::TextWrapped("Save changes to %s before you %s?",
	                   display_document_name(app_state).c_str(),
	                   document_replacement_description(workflow.pendingAction()));
	ImGui::Spacing();
	if (ImGui::Button("Save")) {
		const DocumentSaveAttempt save_attempt = save_current_document(app_state, true);
		if (save_attempt == DocumentSaveAttempt::Saved) {
			perform_document_replacement_action(app_state, workflow.authorizeReplacement());
			ImGui::CloseCurrentPopup();
		} else if (save_attempt == DocumentSaveAttempt::DialogRequested) {
			ImGui::CloseCurrentPopup();
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Discard")) {
		perform_document_replacement_action(app_state, workflow.authorizeReplacement());
		ImGui::CloseCurrentPopup();
	}
	ImGui::SameLine();
	if (ImGui::Button("Cancel")) {
		workflow.cancelReplacement();
		if (running_application_window != nullptr) {
			glfwSetWindowShouldClose(running_application_window, GLFW_FALSE);
		}
		ImGui::CloseCurrentPopup();
	}
	ImGui::EndPopup();
}

void process_pending_local_file_dialog(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr || running_local_file_dialog_service == nullptr) {
		return;
	}
	std::optional<LocalFileDialogRequest> pending_request =
		app_state->local_document_workflow.takePendingDialogRequest();
	if (!pending_request.has_value()) {
		return;
	}

	const LocalFileDialogResult result =
		running_local_file_dialog_service->show(*pending_request);
	if (result.outcome() == LocalFileDialogOutcome::Cancelled) {
		if (pending_request->purpose ==
		    LocalFileDialogPurpose::SaveBeforeDocumentReplacement) {
			app_state->document_replacement_workflow.cancelReplacement();
			set_local_document_feedback(
				app_state,
				"Save canceled; the current document was kept open.",
				false);
		}
		return;
	}
	if (result.outcome() == LocalFileDialogOutcome::Failed) {
		set_local_document_feedback(app_state, result.errorMessage(), true);
		if (pending_request->purpose ==
		    LocalFileDialogPurpose::SaveBeforeDocumentReplacement) {
			app_state->document_replacement_workflow.cancelReplacement();
		}
		return;
	}

	if (pending_request->purpose == LocalFileDialogPurpose::OpenGrammar) {
		open_document_from_local_path(app_state, result.selectedPath());
		return;
	}

	if (!save_document_to_local_path(app_state, result.selectedPath())) {
		if (pending_request->purpose ==
		    LocalFileDialogPurpose::SaveBeforeDocumentReplacement) {
			app_state->document_replacement_workflow.cancelReplacement();
		}
		return;
	}
	if (pending_request->purpose ==
	    LocalFileDialogPurpose::SaveBeforeDocumentReplacement) {
		perform_document_replacement_action(
			app_state,
			app_state->document_replacement_workflow.authorizeReplacement());
	}
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

std::string trim_code_copy(const std::string &line)
{
	const std::size_t comment_start = find_comment_start(line);
	if (comment_start == std::string::npos) {
		return trim_copy(line);
	}
	return trim_copy(line.substr(0, comment_start));
}

bool is_blank_or_comment_line(const std::string &line)
{
	return trim_code_copy(line).empty();
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
	return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || c == '.';
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

void add_app_state_grammar_diagnostic(EditorWorkspaceSession *app_state, const GrammarDiagnostic &diagnostic)
{
	if (app_state == nullptr) {
		return;
	}
	app_state->document_diagnostics.addDiagnostic(diagnostic);
}

int line_code_end_column(const std::string &line)
{
	std::size_t code_end = find_comment_start(line);
	if (code_end == std::string::npos) {
		code_end = line.size();
	}
	while (code_end > 0 &&
	       std::isspace(static_cast<unsigned char>(line[code_end - 1])) != 0) {
		--code_end;
	}
	return static_cast<int>(code_end);
}

GrammarDiagnostic make_line_grammar_diagnostic(const std::vector<std::string> &lines,
                                               int line_number,
                                               const std::string &message)
{
	GrammarDiagnostic diagnostic;
	diagnostic.line = line_number;
	diagnostic.message = message;
	if (line_number >= 0 && line_number < static_cast<int>(lines.size())) {
		const std::string &line = lines[static_cast<std::size_t>(line_number)];
		diagnostic.start_column = first_non_whitespace_column(line);
		diagnostic.end_column =
			std::max(diagnostic.start_column + 1, line_code_end_column(line));
	}
	return diagnostic;
}

GrammarDiagnostic make_token_grammar_diagnostic(const ValidationTokenSpan &token,
                                                const std::string &message)
{
	GrammarDiagnostic diagnostic;
	diagnostic.line = token.line;
	diagnostic.start_column = token.start_column;
	diagnostic.end_column = std::max(token.start_column + 1, token.end_column);
	diagnostic.message = message;
	return diagnostic;
}

std::vector<ValidationTokenSpan> tokenize_validation_rule_block(const std::vector<std::string> &lines,
                                                                int start_line,
                                                                int end_line)
{
	std::vector<ValidationTokenSpan> tokens;
	if (start_line < 0 || end_line < start_line) {
		return tokens;
	}

	for (int line_number = start_line;
	     line_number <= end_line &&
	     line_number < static_cast<int>(lines.size());
	     ++line_number) {
		const std::string &line = lines[static_cast<std::size_t>(line_number)];
		const std::size_t code_end_raw = find_comment_start(line);
		const std::size_t code_end =
			code_end_raw == std::string::npos ? line.size() : code_end_raw;
		const std::size_t code_start = line.find_first_not_of(" \t\r\n");
		if (code_start == std::string::npos || code_start >= code_end) {
			continue;
		}

		std::string current_token;
		int current_start_column = -1;
		const auto flush_token = [&]() {
			if (current_token.empty()) {
				return;
			}
			tokens.push_back({current_token,
			                 line_number,
			                 current_start_column,
			                 current_start_column + static_cast<int>(current_token.size())});
			current_token.clear();
			current_start_column = -1;
		};

		for (std::size_t column = code_start; column < code_end; ++column) {
			const char c = line[column];
			if (std::isspace(static_cast<unsigned char>(c)) != 0) {
				flush_token();
				continue;
			}
			if (c == '-' && column + 1 < code_end && line[column + 1] == '>') {
				flush_token();
				tokens.push_back({"->",
				                 line_number,
				                 static_cast<int>(column),
				                 static_cast<int>(column) + 2});
				++column;
				continue;
			}
			if (c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
			    c == '|' || c == ';' || c == '?' || c == ':') {
				flush_token();
				tokens.push_back({std::string(1, c),
				                 line_number,
				                 static_cast<int>(column),
				                 static_cast<int>(column) + 1});
				continue;
			}
			if (current_token.empty()) {
				current_start_column = static_cast<int>(column);
			}
			current_token += c;
		}
		flush_token();
	}

	return tokens;
}

std::string join_validation_tokens(const std::vector<ValidationTokenSpan> &tokens,
                                   std::size_t start_index,
                                   std::size_t end_index)
{
	if (start_index >= end_index || start_index >= tokens.size()) {
		return "";
	}
	end_index = std::min(end_index, tokens.size());

	std::string output;
	for (std::size_t index = start_index; index < end_index; ++index) {
		if (!output.empty()) {
			output += ' ';
		}
		output += tokens[index].text;
	}
	return output;
}

std::vector<ValidationRuleBlock> build_validation_rule_blocks(const std::vector<std::string> &lines)
{
	std::vector<ValidationRuleBlock> blocks;
	int current_rule_start_line = -1;

	const auto finalize_block = [&](int end_line) {
		if (current_rule_start_line < 0 || end_line < current_rule_start_line) {
			return;
		}
		ValidationRuleBlock block;
		block.start_line = current_rule_start_line;
		block.end_line = end_line;
		block.tokens = tokenize_validation_rule_block(lines, current_rule_start_line, end_line);
		block.content = join_validation_tokens(block.tokens, 0, block.tokens.size());
		if (!block.content.empty()) {
			blocks.push_back(std::move(block));
		}
		current_rule_start_line = -1;
	};

	for (int line_number = 0; line_number < static_cast<int>(lines.size()); ++line_number) {
		const std::string &line = lines[static_cast<std::size_t>(line_number)];
		const std::size_t code_end_raw = find_comment_start(line);
		const std::size_t code_end =
			code_end_raw == std::string::npos ? line.size() : code_end_raw;
		const std::string trimmed = trim_copy(line.substr(0, code_end));
		if (trimmed.empty()) {
			continue;
		}
		if (current_rule_start_line < 0) {
			current_rule_start_line = line_number;
			continue;
		}
		if (starts_new_rule(trimmed)) {
			finalize_block(line_number - 1);
			current_rule_start_line = line_number;
		}
	}

	finalize_block(static_cast<int>(lines.size()) - 1);
	return blocks;
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
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0x4f9b63));
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
	return token == "+" || token == "-" || token == "*" || token == "/" ||
	       token == "^" || token == "&";
}

bool is_comparison_operator_token(const std::string &token)
{
	return token == "<" || token == ">" || token == "<=" || token == ">=" ||
	       token == "==" || token == "!=";
}

bool is_conditional_operator_token(const std::string &token)
{
	return token == "?" || token == ":";
}

ImU32 color_for_editor_token(const EditorToken &token)
{
	const GrammarSyntaxColorRole color_role =
		grammar_syntax_presentation().colorRoleForToken(token);
	if (color_role == GrammarSyntaxColorRole::NumericExpression ||
	    color_role == GrammarSyntaxColorRole::ArithmeticExpression) {
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0xc98787));
	}
	if (color_role == GrammarSyntaxColorRole::ConditionalExpression) {
		return ImGui::ColorConvertFloat4ToU32(color_from_hex(0x6bbfcb));
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
		return "Rule section separator. Splits setup tokens, the repeated body, and the post-repeat tail.";
	}
	if (token == "[") {
		return "Branch start. Opens a scoped transform or generation block.";
	}
	if (token == "]") {
		return "Branch end. Closes the current scoped generation block.";
	}
	if (token == "(" || token == ")") {
		return "Argument and expression delimiter.";
	}
	if (token == "{") {
		return "Brace delimiter. Highlighted by the editor for structure, but the current grammar runtime does not give braces special block behavior.";
	}
	if (token == "}") {
		return "Brace delimiter. Closes a brace pair in the editor, but the current grammar runtime does not give braces special block behavior.";
	}
	if (token == ",") {
		return "Function-argument separator. Action and rule-call arguments remain whitespace-separated.";
	}
	if (token == ";") {
		return "Probability separator. In `Rule ; p -> primary -> alternate`, the primary branch is taken with probability `p`.";
	}
	if (token == "&") {
		return "Reroll operator. Samples a fresh value from the referenced variable's original min and max range.";
	}
	if (token == "?") {
		return "Conditional operator. In `?(cond_expr) true_rules : false_rules`, expands the left branch when the condition is true.";
	}
	if (token == ":") {
		return "Conditional separator. Starts the false branch of `?(cond_expr) true_rules : false_rules`.";
	}
	if (token == "<" || token == ">" || token == "<=" || token == ">=" ||
	    token == "==" || token == "!=") {
		return "Comparison operator used inside conditional expressions.";
	}
	if (token == "=") {
		return "Equals sign. Conditions use `==` for equality and `!=` for inequality.";
	}
	if (token == "!") {
		return "Exclamation mark. Used by `!I` for immovable instancing and by `!=` for inequality.";
	}
	if (token == "+" || token == "-" || token == "*" || token == "/" || token == "^") {
		return "Arithmetic operator used inside grammar expressions.";
	}
	return "Grammar symbol.";
}

std::string tooltip_for_expression_function(const std::string &token)
{
	if (token == "cycle") {
		return "cycle(time, period[, phase]) returns a repeating 0..1 sawtooth. Time and period are seconds; phase is measured in cycles.";
	}
	if (token == "pingpong") {
		return "pingpong(time, period[, phase]) returns a bounded 0..1 out-and-back triangle cycle over one full period.";
	}
	if (token == "pulse") {
		return "pulse(time, period, duty[, phase]) returns 1 during the duty fraction of each cycle and 0 otherwise.";
	}
	if (token == "accelerate") {
		return "accelerate(progress[, exponent]) clamps progress to 0..1 and applies ease-in acceleration. The default exponent is 2.";
	}
	if (token == "decelerate") {
		return "decelerate(progress[, exponent]) clamps progress to 0..1 and applies ease-out deceleration. The default exponent is 2.";
	}
	if (token == "ease") {
		return "ease(progress) applies cubic smoothstep to clamped 0..1 progress.";
	}
	if (token == "smoother") {
		return "smoother(progress) applies quintic smootherstep to clamped 0..1 progress.";
	}
	if (token == "swing") {
		return "swing(time, minimum, maximum, period[, phase]) returns bounded triangle-wave motion, beginning at minimum.";
	}
	if (token == "oscillate") {
		return "oscillate(time, minimum, maximum, period[, phase]) returns bounded sinusoidal motion around the interval midpoint.";
	}
	if (token == "spin") {
		return "spin(time, rate[, offset]) returns offset + time*rate. For A(), rate and offset are normally degrees per second and degrees.";
	}
	if (token == "clamp") {
		return "clamp(value, minimum, maximum) limits a value to an inclusive interval.";
	}
	if (token == "wrap") {
		return "wrap(value, minimum, maximum) folds a value into the half-open interval [minimum, maximum).";
	}
	if (token == "lerp") {
		return "lerp(start, end, progress) linearly interpolates. Progress is not clamped, so extrapolation is permitted.";
	}
	if (token == "radians") {
		return "radians(degrees) converts degrees to radians for trigonometric expressions.";
	}
	if (token == "degrees") {
		return "degrees(radians) converts radians to degrees. A() consumes degrees.";
	}
	if (token == "sin" || token == "cos" || token == "tan") {
		return token + "(angle) evaluates a trigonometric function using radians.";
	}
	if (token == "min" || token == "max") {
		return token + "(a, b) returns the smaller or larger value.";
	}
	if (token == "sqrt") {
		return "sqrt(value) returns the square root and rejects negative input.";
	}
	return "Checked grammar expression function. Function arguments are comma-separated.";
}

std::string tooltip_for_keyword(const std::string &token)
{
	if (token == "R") {
		return "Random variable declaration. Assigns a sampled value into a named symbol.";
	}
	if (token == "R*") {
		return "Random integer-like variable declaration. Often used for discrete rule or legacy material indices.";
	}
	if (token == "S") {
		return "Scale transform.";
	}
	if (token == "T") {
		return "Translate transform.";
	}
	if (token == "I") {
		return "Instantiate Cube families, canonical Cylinder(...), Sphere(...), or AxialProfile(...) descriptors, procedural aliases, and STL catalog geometry.";
	}
	if (token == "!I") {
		return "Instantiate immovable geometry. Pending V() and VR() are ignored.";
	}
	if (token == "V") {
		return "Assign initial linear velocity to the next instantiated primitive.";
	}
	if (token == "VR") {
		return "Assign initial rotational velocity to the next instantiated primitive.";
	}
	if (token == "M") {
		return "Assign mass to the next instantiated primitive. If P() is also present, density-derived mass wins.";
	}
	if (token == "P") {
		return "Assign density to the next closed primitive. Curved mass uses validated analytic or watertight-mesh volume and rejects open surfaces.";
	}
	if (token == "G") {
		return "Set global gravity. The first argument is magnitude and the next three arguments define the direction. Later G() calls override earlier ones.";
	}
	if (token == "A") {
		return "Axis-angle rotation. Use A(angle axis), or A(angle axis lower upper) to clamp the evaluated angle. Axis is exactly 0, 1, or 2 for X, Y, or Z.";
	}
	if (token == "D") {
		return "Secondary scale transform. Affects the alternate half-transform used by split cube primitives.";
	}
	if (token == "DSX") {
		return "Dual-scale transform for the primitive's minimum X face. Defaults to (1, 1, 1) when omitted.";
	}
	if (token == "DSY") {
		return "Dual-scale transform for the primitive's minimum Y face. Defaults to (1, 1, 1) when omitted.";
	}
	if (token == "DSZ") {
		return "Dual-scale transform for the primitive's minimum Z face. Defaults to (1, 1, 1) when omitted.";
	}
	if (token == "DTX") {
		return "Dual-translation transform for the primitive's minimum X face. Defaults to (0, 0, 0) when omitted.";
	}
	if (token == "DTY") {
		return "Dual-translation transform for the primitive's minimum Y face. Defaults to (0, 0, 0) when omitted.";
	}
	if (token == "DTZ") {
		return "Dual-translation transform for the primitive's minimum Z face. Defaults to (0, 0, 0) when omitted.";
	}
	return "Grammar keyword.";
}

std::string tooltip_for_primitive_symbol(const std::string &token)
{
	const ProceduralShapeCatalogRepository procedural_catalog;
	const std::string procedural_tooltip = procedural_catalog.tooltipFor(token);
	if (!procedural_tooltip.empty()) {
		return procedural_tooltip;
	}
	const std::string lowered = lowercase_copy(token);
	if (lowered.rfind("stl.", 0) == 0 && token.size() > 4) {
		const auto entry_it = stl_catalog_lookup.find(lowered.substr(4));
		if (entry_it != stl_catalog_lookup.end()) {
			if (!entry_it->second.tooltip.empty()) {
				return entry_it->second.tooltip;
			}
			return "STL mesh loaded from stls/" + entry_it->second.name +
			       ".stl (category: " + entry_it->second.category + ").";
		}
		std::string file_name = token.substr(4);
		const std::size_t separator = file_name.find('.');
		if (separator != std::string::npos && separator + 1 < file_name.size()) {
			file_name = file_name.substr(separator + 1);
		}
		return "STL mesh loaded from stls/" + file_name + ".stl.";
	}
	return "Built-in primitive mesh symbol used by the geometry instancer.";
}

GrammarSyntaxPresentation &grammar_syntax_presentation()
{
	static GrammarSyntaxPresentation presentation(
		[](const std::string &identifier) {
			return part_class_name_set.count(identifier) != 0U;
		},
		tooltip_for_operator,
		tooltip_for_keyword,
		tooltip_for_expression_function,
		tooltip_for_primitive_symbol);
	return presentation;
}

bool is_selectable_identifier_token(EditorTokenKind kind)
{
	return grammar_syntax_presentation().isSelectableIdentifier(kind);
}

bool is_matchable_identifier_token(EditorTokenKind kind)
{
	return grammar_syntax_presentation().isMatchableIdentifier(kind);
}

std::string label_for_editor_token_kind(EditorTokenKind kind)
{
	return grammar_syntax_presentation().labelForTokenKind(kind);
}

bool editor_token_has_tooltip(const EditorToken &token)
{
	return grammar_syntax_presentation().hasTooltip(token);
}

std::vector<EditorToken> tokenize_grammar_line(const std::string &line, const GrammarSymbolIndex &symbols)
{
	return grammar_syntax_presentation().tokenizeLine(line, symbols);
}

std::vector<EditorTokenSpan> tokenize_grammar_line_with_spans(const std::string &line,
                                                              const GrammarSymbolIndex &symbols)
{
	return grammar_syntax_presentation().tokenizeLineWithSpans(line, symbols);
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
                                         int column,
                                         const GrammarSymbolIndex *symbols = nullptr)
{
	column = std::clamp(column, 0, static_cast<int>(line.size()));
	float x_offset = 0.0f;
	if (symbols == nullptr) {
		const char *line_start = line.c_str();
		const char *line_end = line_start + column;
		x_offset = ImGui::CalcTextSize(line_start, line_end, false, -1.0f).x;
	} else {
		const std::vector<EditorTokenSpan> spans = tokenize_grammar_line_with_spans(line, *symbols);
		for (const EditorTokenSpan &span : spans) {
			if (column <= span.start_column) {
				break;
			}

			const int local_end = std::min(column, span.end_column) - span.start_column;
			if (local_end <= 0) {
				continue;
			}

			const char *token_start = span.token.text.c_str();
			const char *token_end = token_start + local_end;
			x_offset += ImGui::CalcTextSize(token_start, token_end, false, -1.0f).x;

			if (column < span.end_column) {
				break;
			}
		}
	}
	return ImVec2(layout.content_origin.x + x_offset,
	              layout.content_origin.y + line_index * layout.line_height);
}

ImRect editor_rect_for_range(const EditorOverlayLayout &layout,
                             const std::vector<std::string> &lines,
                             const EditorRange &range,
                             const GrammarSymbolIndex *symbols = nullptr)
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

	const ImVec2 min =
		editor_screen_position_for_column(layout, line, range.line, start_column, symbols);
	ImVec2 max =
		editor_screen_position_for_column(layout, line, range.line, end_column, symbols);
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
	for (const EditorTokenSpan &span : spans) {
		if (span.start_column == range.start_column && span.end_column == range.end_column) {
			const ImVec2 min =
				editor_screen_position_for_column(layout, line, range.line, span.start_column, &symbols);
			ImVec2 max =
				editor_screen_position_for_column(layout, line, range.line, span.end_column, &symbols);
			if (max.x <= min.x) {
				max.x = min.x + std::max(1.0f, layout.font_size * 0.55f);
			}
			max.y = min.y + layout.line_height;
			*rect_out = ImRect(min, max);
			return true;
		}
	}

	return false;
}

bool find_hovered_editor_token(const std::vector<std::string> &lines,
                               const GrammarSymbolIndex &symbols,
                               ImGuiID editor_id,
                               const ImRect &editor_rect,
                               EditorToken *token_out,
                               EditorRange *range_out)
{
	if (token_out == nullptr || range_out == nullptr || editor_id == 0 || lines.empty()) {
		return false;
	}

	EditorOverlayLayout layout;
	if (!build_editor_overlay_layout(editor_id, editor_rect, &layout)) {
		return false;
	}

	const ImVec2 mouse_position = ImGui::GetIO().MousePos;
	if (!layout.clip_rect.Contains(mouse_position)) {
		return false;
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

	for (int line_index = first_visible_line; line_index < last_visible_line; ++line_index) {
		const std::string &line = lines[static_cast<std::size_t>(line_index)];
		const std::vector<EditorTokenSpan> spans = tokenize_grammar_line_with_spans(line, symbols);
		for (const EditorTokenSpan &span : spans) {
			if (!editor_token_has_tooltip(span.token)) {
				continue;
			}

			EditorRange range;
			range.line = line_index;
			range.start_column = span.start_column;
			range.end_column = span.end_column;
			range.found = true;

			ImRect rect;
			if (!editor_rect_for_token_range(layout, lines, symbols, range, &rect)) {
				rect = editor_rect_for_range(layout, lines, range, &symbols);
			}
			if (!rect.Contains(mouse_position)) {
				continue;
			}

			*token_out = span.token;
			*range_out = range;
			return true;
		}
	}

	return false;
}

void draw_editor_diagnostic_underline(const EditorOverlayLayout &layout,
                                      const std::vector<std::string> &lines,
                                      const EditorDiagnostic &diagnostic,
                                      const GrammarSymbolIndex *symbols = nullptr)
{
	const ImRect rect = editor_rect_for_range(layout, lines, diagnostic.range, symbols);
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

std::vector<EditorDiagnostic> build_editor_diagnostics(const std::vector<std::string> &lines, EditorWorkspaceSession *app_state = nullptr)
{
	static const GrammarDiagnosticPresentation presentation;
	return presentation.buildDiagnostics(
		lines,
		app_state == nullptr ? nullptr : &app_state->document_diagnostics);
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

bool draw_editor_diagnostic_tooltip(const std::vector<std::string> &lines,
                                    const GrammarSymbolIndex &symbols,
                                    const std::vector<EditorDiagnostic> &diagnostics,
                                    ImGuiID editor_id,
                                    const ImRect &editor_rect)
{
	if (diagnostics.empty()) {
		return false;
	}

	EditorOverlayLayout layout;
	if (!build_editor_overlay_layout(editor_id, editor_rect, &layout)) {
		return false;
	}

	const ImVec2 mouse_position = ImGui::GetIO().MousePos;
	for (const EditorDiagnostic &diagnostic : diagnostics) {
		const ImRect rect = editor_rect_for_range(layout, lines, diagnostic.range, &symbols);
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
		return true;
	}
	return false;
}

void draw_editor_caret_overlay(const std::vector<std::string> &lines,
                               const GrammarSymbolIndex &symbols,
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
		editor_screen_position_for_column(layout,
		                                  line,
		                                  cursor_position.line,
		                                  cursor_position.column,
		                                  &symbols);
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
	std::unordered_set<int> marked_diagnostic_lines;
	for (const EditorDiagnostic &diagnostic : diagnostics) {
		if (!diagnostic.range.found || diagnostic.range.line < first_visible_line ||
		    diagnostic.range.line >= last_visible_line ||
		    !marked_diagnostic_lines.insert(diagnostic.range.line).second) {
			continue;
		}
		const float marker_x = layout.clip_rect.Min.x + 4.0f;
		const float marker_y = layout.content_origin.y +
		                       (static_cast<float>(diagnostic.range.line) + 0.5f) *
		                           layout.line_height;
		layout.draw_list->AddCircleFilled(ImVec2(marker_x, marker_y),
		                                  2.75f,
		                                  color_for_diagnostic(diagnostic.severity));
	}

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
		const std::string &line = lines[static_cast<std::size_t>(line_index)];
		const std::vector<EditorTokenSpan> spans = tokenize_grammar_line_with_spans(line, symbols);
		for (const EditorTokenSpan &span : spans) {
			const EditorToken &token = span.token;
			if (token.kind != EditorTokenKind::Plain && !token.text.empty()) {
				const ImVec2 token_pos =
					editor_screen_position_for_column(layout, line, line_index, span.start_column, &symbols);
				const ImU32 token_color = color_for_editor_token(token);
				layout.draw_list->AddText(ImGui::GetFont(),
				                          layout.font_size,
				                          token_pos,
				                          token_color,
				                          token.text.c_str());
				if (is_conditional_operator_token(token.text) ||
				    is_comparison_operator_token(token.text) ||
				    token.text == "=") {
					const float bold_offset = std::max(0.6f, layout.font_size * 0.06f);
					layout.draw_list->AddText(ImGui::GetFont(),
					                          layout.font_size,
					                          ImVec2(token_pos.x + bold_offset, token_pos.y),
					                          token_color,
					                          token.text.c_str());
				}
			}
		}
	}

	for (const EditorDiagnostic &diagnostic : diagnostics) {
		if (!diagnostic.range.found || diagnostic.range.line < first_visible_line ||
		    diagnostic.range.line >= last_visible_line) {
			continue;
		}
		draw_editor_diagnostic_underline(layout, lines, diagnostic, &symbols);
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

bool starts_with_case_insensitive(const std::string &value, const std::string &prefix)
{
	return grammar_autocomplete_presentation.startsWithIgnoringCase(value, prefix);
}

bool is_stl_autocomplete_prefix(const std::string &prefix)
{
	return grammar_autocomplete_presentation.isStlQualifiedPrefix(prefix);
}

std::string stl_leaf_name_from_qualified_name(const std::string &qualified_name)
{
	const std::size_t separator = qualified_name.find('.');
	if (separator == std::string::npos || separator + 1 >= qualified_name.size()) {
		return qualified_name;
	}
	return qualified_name.substr(separator + 1);
}

const StlCategoryTreeNode *find_exact_stl_category_child(const StlCategoryTreeNode &node,
                                                         const std::string &token)
{
	for (const StlCategoryTreeNode &child : node.children) {
		if (lowercase_copy(child.name) == lowercase_copy(token)) {
			return &child;
		}
	}
	return nullptr;
}

const StlCategoryTreeNode *find_unique_prefixed_stl_category_child(const StlCategoryTreeNode &node,
                                                                   const std::string &token)
{
	const StlCategoryTreeNode *match = nullptr;
	for (const StlCategoryTreeNode &child : node.children) {
		if (!starts_with_case_insensitive(child.name, token)) {
			continue;
		}
		if (match != nullptr) {
			return nullptr;
		}
		match = &child;
	}
	return match;
}

std::vector<const StlCategoryTreeNode *> find_prefixed_stl_category_children(const StlCategoryTreeNode &node,
                                                                             const std::string &token)
{
	std::vector<const StlCategoryTreeNode *> matches;
	for (const StlCategoryTreeNode &child : node.children) {
		if (token.empty() || starts_with_case_insensitive(child.name, token)) {
			matches.push_back(&child);
		}
	}
	return matches;
}

bool populate_stl_autocomplete_from_tree(const std::string &prefix, PartClassAutocomplete *autocomplete)
{
	if (autocomplete == nullptr || !has_stl_category_tree || !is_stl_autocomplete_prefix(prefix)) {
		return false;
	}

	const std::string lowered = lowercase_copy(prefix);
	const std::string suffix = lowered == "stl" ? "" : (lowered == "stl." ? "" : prefix.substr(4));
	autocomplete->display_prefix = suffix;

	const std::vector<std::string> raw_tokens = split_string_preserve_empty(suffix, '.');
	const bool trailing_dot = !suffix.empty() && suffix.back() == '.';
	const int resolved_token_count =
		trailing_dot ? std::max(0, static_cast<int>(raw_tokens.size()) - 1)
		             : std::max(0, static_cast<int>(raw_tokens.size()) - 1);

	const StlCategoryTreeNode *cursor = &stl_category_tree_root;
	std::vector<std::string> resolved_tokens;
	for (int index = 0; index < resolved_token_count; ++index) {
		if (raw_tokens[static_cast<std::size_t>(index)].empty()) {
			return false;
		}
		const StlCategoryTreeNode *child =
			find_exact_stl_category_child(*cursor, raw_tokens[static_cast<std::size_t>(index)]);
		if (child == nullptr) {
			child = find_unique_prefixed_stl_category_child(*cursor, raw_tokens[static_cast<std::size_t>(index)]);
		}
		if (child == nullptr) {
			return false;
		}
		cursor = child;
		resolved_tokens.push_back(child->name);
	}

	const std::string current_token =
		trailing_dot ? "" : raw_tokens.empty() ? "" : raw_tokens.back();
	const std::vector<const StlCategoryTreeNode *> child_matches =
		find_prefixed_stl_category_children(*cursor, current_token);
	if (!child_matches.empty()) {
		autocomplete->stl_category_context = true;
		for (const StlCategoryTreeNode *child : child_matches) {
			if (child == nullptr) {
				continue;
			}
			std::vector<std::string> suggestion_tokens = resolved_tokens;
			suggestion_tokens.push_back(child->name);
			autocomplete->suggestions.push_back("STL." + join_string_tokens(suggestion_tokens, '.') + ".");
		}
		return true;
	}

	if (!cursor->names.empty()) {
		autocomplete->stl_category_context = false;
		const std::string base_path = join_string_tokens(resolved_tokens, '.');
		for (const std::string &name : cursor->names) {
			if (!current_token.empty() && !starts_with_case_insensitive(name, current_token)) {
				continue;
			}
			autocomplete->suggestions.push_back(
				"STL." + (base_path.empty() ? name : base_path + "." + name));
		}
		return !autocomplete->suggestions.empty();
	}

	return false;
}

std::string resolve_stl_category_name(const std::string &category_query)
{
	return grammar_autocomplete_presentation.resolveUniqueStlCategory(
		category_query,
		stl_category_names);
}

StlAutocompleteQuery parse_stl_autocomplete_query(const std::string &prefix)
{
	return grammar_autocomplete_presentation.parseStlQuery(prefix);
}

struct InlineAutocompleteTokenSpan {
	std::size_t start = 0;
	std::size_t end = 0;
	std::string text;
};

std::size_t find_active_instance_call_open_paren(const std::string &line, int cursor_column)
{
	std::size_t call_open_paren = std::string::npos;
	for (std::size_t index = 0; index < static_cast<std::size_t>(cursor_column); ++index) {
		const bool is_bang_instance =
			line.compare(index, 2, "!I") == 0 &&
			(index + 2 >= line.size() || !is_identifier_char(line[index + 2]));
		const bool is_plain_instance =
			line[index] == 'I' &&
			(index == 0 || !is_identifier_char(line[index - 1])) &&
			(index + 1 >= line.size() || !is_identifier_char(line[index + 1]));
		if (!is_bang_instance && !is_plain_instance) {
			continue;
		}

		std::size_t token_end = index + (is_bang_instance ? 2 : 1);
		while (token_end < line.size() &&
		       std::isspace(static_cast<unsigned char>(line[token_end])) != 0) {
			++token_end;
		}
		if (token_end >= line.size() || line[token_end] != '(') {
			continue;
		}

		if (token_end + 1 < static_cast<std::size_t>(cursor_column) &&
		    line.substr(token_end + 1,
		                static_cast<std::size_t>(cursor_column) - token_end - 1).find(')') != std::string::npos) {
			continue;
		}

		call_open_paren = token_end;
	}
	return call_open_paren;
}

std::vector<InlineAutocompleteTokenSpan> parse_inline_instance_tokens(const std::string &line,
                                                                      std::size_t call_open_paren,
                                                                      std::size_t *close_paren_column)
{
	std::vector<InlineAutocompleteTokenSpan> tokens;
	std::size_t cursor = std::min(call_open_paren + 1, line.size());
	std::size_t resolved_close = line.size();
	while (cursor < line.size()) {
		while (cursor < line.size() &&
		       std::isspace(static_cast<unsigned char>(line[cursor])) != 0) {
			++cursor;
		}
		if (cursor >= line.size()) {
			break;
		}
		if (line[cursor] == ')') {
			resolved_close = cursor;
			break;
		}
		const std::size_t start = cursor;
		while (cursor < line.size() &&
		       std::isspace(static_cast<unsigned char>(line[cursor])) == 0 &&
		       line[cursor] != ')') {
			++cursor;
		}
		tokens.push_back({start, cursor, line.substr(start, cursor - start)});
		if (cursor < line.size() && line[cursor] == ')') {
			resolved_close = cursor;
			break;
		}
	}
	if (close_paren_column != nullptr) {
		*close_paren_column = resolved_close;
	}
	return tokens;
}

bool looks_like_material_autocomplete_token(const std::string &token)
{
	if (token.empty()) {
		return false;
	}
	bool has_alpha = false;
	for (unsigned char character : token) {
		if (std::isalpha(character) != 0) {
			has_alpha = true;
		}
		if (character == '+' || character == '-' || character == '*' || character == '/') {
			return false;
		}
	}
	return has_alpha;
}

MaterialAutocomplete build_material_autocomplete(const std::vector<std::string> &lines,
                                                 const EditorTokenMatch &cursor_position)
{
	MaterialAutocomplete autocomplete;
	if (!cursor_position.found || cursor_position.line < 0 ||
	    cursor_position.line >= static_cast<int>(lines.size())) {
		return autocomplete;
	}

	const std::string &line = lines[static_cast<std::size_t>(cursor_position.line)];
	const int cursor_column =
		std::clamp(cursor_position.column, 0, static_cast<int>(line.size()));
	const std::size_t call_open_paren =
		find_active_instance_call_open_paren(line, cursor_column);
	if (call_open_paren == std::string::npos) {
		return autocomplete;
	}

	std::size_t close_paren_column = line.size();
	const std::vector<InlineAutocompleteTokenSpan> tokens =
		parse_inline_instance_tokens(line, call_open_paren, &close_paren_column);
	if (tokens.empty()) {
		return autocomplete;
	}

	const InlineAutocompleteTokenSpan &primitive_token = tokens.front();
	if (static_cast<std::size_t>(cursor_column) <= primitive_token.start ||
	    static_cast<std::size_t>(cursor_column) < primitive_token.end) {
		return autocomplete;
	}

	std::size_t replace_start = close_paren_column;
	std::size_t replace_end = close_paren_column;
	std::string prefix;
	bool append_trailing_space = false;
	bool cursor_in_material_slot = false;

	if (tokens.size() >= 2) {
		const InlineAutocompleteTokenSpan &candidate_token = tokens[1];
		if (looks_like_material_autocomplete_token(candidate_token.text)) {
			replace_start = candidate_token.start;
			replace_end = candidate_token.end;
			cursor_in_material_slot =
				cursor_column >= static_cast<int>(primitive_token.end) &&
				cursor_column <= static_cast<int>(candidate_token.end);
			if (!cursor_in_material_slot) {
				return autocomplete;
			}
			if (cursor_column > static_cast<int>(candidate_token.start)) {
				const std::size_t prefix_end =
					std::min(static_cast<std::size_t>(cursor_column), candidate_token.end);
				prefix =
					line.substr(candidate_token.start, prefix_end - candidate_token.start);
			}
		} else {
			replace_start = candidate_token.start;
			replace_end = candidate_token.start;
			append_trailing_space = true;
			cursor_in_material_slot =
				cursor_column >= static_cast<int>(primitive_token.end) &&
				cursor_column <= static_cast<int>(candidate_token.start);
			if (!cursor_in_material_slot) {
				return autocomplete;
			}
		}
	} else {
		replace_start = close_paren_column;
		replace_end = close_paren_column;
		append_trailing_space = true;
		cursor_in_material_slot =
			cursor_column >= static_cast<int>(primitive_token.end) &&
			cursor_column <= static_cast<int>(close_paren_column);
		if (!cursor_in_material_slot) {
			return autocomplete;
		}
	}

	const MaterialAutocompletePrefixState prefix_state =
		analyze_material_autocomplete_prefix(prefix);
	const auto &lexemes = material_lexemes();
	for (MaterialLexemeKind kind : ordered_material_autocomplete_kinds(prefix_state)) {
		std::vector<MaterialAutocompleteSuggestion> kind_suggestions;
		for (const MaterialLexeme &lexeme : lexemes) {
			if (lexeme.kind != kind) {
				continue;
			}
			if (kind == MaterialLexemeKind::Color && prefix_state.color_count >= 3) {
				continue;
			}
			if (kind == MaterialLexemeKind::Opacity && prefix_state.has_opacity) {
				continue;
			}
			if (kind == MaterialLexemeKind::Family && prefix_state.has_family) {
				continue;
			}
			if (material_prefix_contains_lexeme(prefix_state, lexeme.token)) {
				continue;
			}
			if (!prefix_state.trailing_fragment.empty() &&
			    !starts_with_case_insensitive(std::string(lexeme.token), prefix_state.trailing_fragment)) {
				continue;
			}

			MaterialAutocompleteSuggestion suggestion;
			suggestion.label = format_material_lexeme_label(lexeme.token);
			suggestion.replacement =
				build_material_autocomplete_replacement(prefix_state, lexeme);
			suggestion.kind = lexeme.kind;
			suggestion.swatch = lexeme.swatch;
			kind_suggestions.push_back(std::move(suggestion));
		}
		std::sort(kind_suggestions.begin(),
		          kind_suggestions.end(),
		          [](const MaterialAutocompleteSuggestion &lhs,
		             const MaterialAutocompleteSuggestion &rhs) {
		              return lhs.label < rhs.label;
		          });
		autocomplete.suggestions.insert(autocomplete.suggestions.end(),
		                                kind_suggestions.begin(),
		                                kind_suggestions.end());
	}

	if (autocomplete.suggestions.empty()) {
		return {};
	}

	autocomplete.line = cursor_position.line;
	autocomplete.replace_start_column = static_cast<int>(replace_start);
	autocomplete.replace_end_column = static_cast<int>(replace_end);
	autocomplete.prefix = prefix;
	autocomplete.active = true;
	autocomplete.append_trailing_space = append_trailing_space;
	return autocomplete;
}

PartClassAutocomplete build_part_class_autocomplete(const std::vector<std::string> &lines,
                                                    const EditorTokenMatch &cursor_position)
{
	PartClassAutocomplete autocomplete;
	if (!cursor_position.found || cursor_position.line < 0 ||
	    cursor_position.line >= static_cast<int>(lines.size()) ||
	    part_class_names.empty()) {
		return autocomplete;
	}

	const std::string &line = lines[static_cast<std::size_t>(cursor_position.line)];
	const int cursor_column =
		std::clamp(cursor_position.column, 0, static_cast<int>(line.size()));

	std::size_t call_open_paren = std::string::npos;
	for (std::size_t index = 0; index < static_cast<std::size_t>(cursor_column); ++index) {
		const bool is_bang_instance =
			line.compare(index, 2, "!I") == 0 &&
			(index + 2 >= line.size() || !is_identifier_char(line[index + 2]));
		const bool is_plain_instance =
			line[index] == 'I' &&
			(index == 0 || !is_identifier_char(line[index - 1])) &&
			(index + 1 >= line.size() || !is_identifier_char(line[index + 1]));
		if (!is_bang_instance && !is_plain_instance) {
			continue;
		}

		std::size_t token_end = index + (is_bang_instance ? 2 : 1);
		while (token_end < line.size() &&
		       std::isspace(static_cast<unsigned char>(line[token_end])) != 0) {
			++token_end;
		}
		if (token_end >= line.size() || line[token_end] != '(') {
			continue;
		}

		if (token_end + 1 < static_cast<std::size_t>(cursor_column) &&
		    line.substr(token_end + 1,
		                static_cast<std::size_t>(cursor_column) - token_end - 1).find(')') != std::string::npos) {
			continue;
		}

		call_open_paren = token_end;
	}

	if (call_open_paren == std::string::npos) {
		return autocomplete;
	}

	std::size_t class_start = call_open_paren + 1;
	while (class_start < line.size() &&
	       std::isspace(static_cast<unsigned char>(line[class_start])) != 0) {
		++class_start;
	}
	if (static_cast<std::size_t>(cursor_column) < class_start) {
		return autocomplete;
	}

	std::size_t class_end = class_start;
	while (class_end < line.size() &&
	       std::isspace(static_cast<unsigned char>(line[class_end])) == 0 &&
	       line[class_end] != ')') {
		++class_end;
	}
	if (static_cast<std::size_t>(cursor_column) > class_end) {
		return autocomplete;
	}

	autocomplete.line = cursor_position.line;
	autocomplete.replace_start_column = static_cast<int>(class_start);
	autocomplete.replace_end_column = static_cast<int>(class_end);
	autocomplete.prefix = line.substr(class_start, static_cast<std::size_t>(cursor_column) - class_start);
	autocomplete.active = true;

	autocomplete.suggestions = grammar_autocomplete_presentation.findPartClassSuggestions(
		autocomplete.prefix,
		part_class_names);
	if (autocomplete.suggestions.empty()) {
		return {};
	}

	return autocomplete;
}

PartClassAutocomplete build_shape_option_autocomplete(
	const std::vector<std::string> &lines,
	const EditorTokenMatch &cursor_position)
{
	PartClassAutocomplete autocomplete;
	if (!cursor_position.found || cursor_position.line < 0 ||
	    cursor_position.line >= static_cast<int>(lines.size())) {
		return autocomplete;
	}

	const std::string &line = lines[static_cast<std::size_t>(cursor_position.line)];
	const std::size_t cursor_column = static_cast<std::size_t>(
		std::clamp(cursor_position.column, 0, static_cast<int>(line.size())));
	std::ostringstream source_before_cursor;
	for (int line_index = 0; line_index < cursor_position.line; ++line_index) {
		source_before_cursor << lines[static_cast<std::size_t>(line_index)] << '\n';
	}
	source_before_cursor << line.substr(0, cursor_column);
	const ShapeCompletionContext completion_context =
		StructuredShapeCompletionService().analyze(source_before_cursor.str());
	if (!completion_context.isActive()) return autocomplete;

	std::size_t replace_start = cursor_column;
	while (replace_start > 0 && is_identifier_char(line[replace_start - 1])) {
		--replace_start;
	}
	std::size_t replace_end = cursor_column;
	while (replace_end < line.size() && is_identifier_char(line[replace_end])) {
		++replace_end;
	}
	if (replace_start > 0 &&
	    std::isspace(static_cast<unsigned char>(line[replace_start - 1])) == 0 &&
	    line[replace_start - 1] != ')' && line[replace_start - 1] != '(') {
		return autocomplete;
	}

	autocomplete.prefix = line.substr(replace_start, cursor_column - replace_start);
	autocomplete.suggestions =
		grammar_autocomplete_presentation.findShapeOptionSuggestions(
			autocomplete.prefix, completion_context.completions());
	if (autocomplete.suggestions.empty()) return {};
	autocomplete.line = cursor_position.line;
	autocomplete.replace_start_column = static_cast<int>(replace_start);
	autocomplete.replace_end_column = static_cast<int>(replace_end);
	autocomplete.active = true;
	autocomplete.shape_option_context = true;
	return autocomplete;
}

PartClassAutocomplete build_spatial_declaration_autocomplete(
	const std::vector<std::string> &lines,
	const EditorTokenMatch &cursor_position)
{
	PartClassAutocomplete autocomplete;
	if (!cursor_position.found || cursor_position.line < 0 ||
	    cursor_position.line >= static_cast<int>(lines.size())) {
		return autocomplete;
	}

	const std::string &line = lines[static_cast<std::size_t>(cursor_position.line)];
	const std::size_t cursor_column = static_cast<std::size_t>(
		std::clamp(cursor_position.column, 0, static_cast<int>(line.size())));
	std::ostringstream source_before_cursor;
	for (int line_index = 0; line_index < cursor_position.line; ++line_index) {
		source_before_cursor << lines[static_cast<std::size_t>(line_index)] << '\n';
	}
	source_before_cursor << line.substr(0, cursor_column);
	const SpatialDeclarationCompletionContext completion_context =
		SpatialDeclarationCompletionService().analyze(source_before_cursor.str());
	if (!completion_context.isActive()) return autocomplete;

	std::size_t replace_start = cursor_column;
	while (replace_start > 0 && is_identifier_char(line[replace_start - 1])) {
		--replace_start;
	}
	std::size_t replace_end = cursor_column;
	while (replace_end < line.size() && is_identifier_char(line[replace_end])) {
		++replace_end;
	}
	autocomplete.prefix = line.substr(replace_start, cursor_column - replace_start);
	autocomplete.suggestions =
		grammar_autocomplete_presentation.findSpatialDeclarationSuggestions(
			autocomplete.prefix, completion_context.completions());
	if (autocomplete.suggestions.empty()) return {};
	autocomplete.line = cursor_position.line;
	autocomplete.replace_start_column = static_cast<int>(replace_start);
	autocomplete.replace_end_column = static_cast<int>(replace_end);
	autocomplete.active = true;
	autocomplete.spatial_declaration_context = true;
	autocomplete.context_label = completion_context.contextName();
	return autocomplete;
}

void apply_part_class_autocomplete(EditorWorkspaceSession *app_state,
                                   const std::vector<std::string> &lines,
                                   const PartClassAutocomplete &autocomplete,
                                   const std::string &replacement)
{
	if (app_state == nullptr || !autocomplete.active) {
		return;
	}

	const int start_offset =
		document_offset_for_position(lines, autocomplete.line, autocomplete.replace_start_column);
	const int end_offset =
		document_offset_for_position(lines, autocomplete.line, autocomplete.replace_end_column);
	std::string updated_text = app_state->document.source_text;
	updated_text.replace(static_cast<std::size_t>(start_offset),
	                     static_cast<std::size_t>(std::max(0, end_offset - start_offset)),
	                     replacement);
	commit_editor_text(app_state, updated_text);

	const int cursor_offset = start_offset + static_cast<int>(replacement.size());
	smart_editor_state.cursor_column = cursor_offset;
	smart_editor_state.selection_start = cursor_offset;
	smart_editor_state.selection_end = cursor_offset;
	smart_editor_state.requested_cursor_column = cursor_offset;
	smart_editor_state.requested_selection_start = cursor_offset;
	smart_editor_state.requested_selection_end = cursor_offset;
	smart_editor_state.request_cursor_sync = true;
	smart_editor_state.request_selection_sync = true;
	smart_editor_state.request_focus = true;
}

void apply_material_autocomplete(EditorWorkspaceSession *app_state,
                                 const std::vector<std::string> &lines,
                                 const MaterialAutocomplete &autocomplete,
                                 const MaterialAutocompleteSuggestion &suggestion)
{
	if (app_state == nullptr || !autocomplete.active) {
		return;
	}

	const int start_offset =
		document_offset_for_position(lines, autocomplete.line, autocomplete.replace_start_column);
	const int end_offset =
		document_offset_for_position(lines, autocomplete.line, autocomplete.replace_end_column);
	std::string replacement = suggestion.replacement;
	if (autocomplete.append_trailing_space) {
		replacement.push_back(' ');
	}
	std::string updated_text = app_state->document.source_text;
	updated_text.replace(static_cast<std::size_t>(start_offset),
	                     static_cast<std::size_t>(std::max(0, end_offset - start_offset)),
	                     replacement);
	commit_editor_text(app_state, updated_text);

	const int cursor_offset =
		start_offset + static_cast<int>(suggestion.replacement.size());
	smart_editor_state.cursor_column = cursor_offset;
	smart_editor_state.selection_start = cursor_offset;
	smart_editor_state.selection_end = cursor_offset;
	smart_editor_state.requested_cursor_column = cursor_offset;
	smart_editor_state.requested_selection_start = cursor_offset;
	smart_editor_state.requested_selection_end = cursor_offset;
	smart_editor_state.request_cursor_sync = true;
	smart_editor_state.request_selection_sync = true;
	smart_editor_state.request_focus = true;
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

void select_preview_instance_in_editor(EditorWorkspaceSession *app_state, Context *context, int instance_index)
{
	if (app_state == nullptr || context == nullptr ||
	    !is_valid_preview_instance_index(context, instance_index)) {
		return;
	}

	const std::optional<SourceRange> source_range =
		current_preview_session().source_associations.sourceRangeForInstance(instance_index);
	if (!source_range.has_value()) {
		return;
	}

	const std::vector<std::string> lines = current_editor_lines(app_state->document.source_text);
	if (source_range->start_line >= static_cast<int>(lines.size())) {
		return;
	}

	const int line_length =
		static_cast<int>(lines[static_cast<std::size_t>(source_range->start_line)].size());
	const int start_column = std::clamp(source_range->start_column, 0, line_length);
	if (source_range->end_line == source_range->start_line &&
	    source_range->end_column > source_range->start_column) {
		select_editor_range(lines,
		                    source_range->start_line,
		                    start_column,
		                    std::clamp(source_range->end_column,
		                               start_column + 1,
		                               line_length + 1));
		return;
	}

	jump_to_editor_location(lines, source_range->start_line, start_column);
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

bool find_runtime_variable_by_name(const std::string &variable_name,
                                   GrammarRuntimeVariableSnapshot *out_snapshot)
{
	if (scene_generation_in_progress() || grammar == nullptr || out_snapshot == nullptr) {
		return false;
	}
	return getGrammarRuntimeVariableSnapshot(grammar, variable_name, out_snapshot);
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
		GrammarRuntimeVariableSnapshot variable;
		if (find_runtime_variable_by_name(selected.text, &variable)) {
			drew_details = true;
			ImGui::Text("Runtime Value: %.3f", variable.value);
			if (variable.defining_rule == "<builtin-time>") {
				ImGui::TextUnformatted("Type: Built-in time input (seconds)");
				ImGui::TextUnformatted("Immutable: change t with Preview Play, Step, Reset Time, or the time scrubber.");
			} else {
				ImGui::Text("Range: %.3f .. %.3f", variable.min, variable.max);
				ImGui::TextUnformatted(variable.integer ? "Type: Integer" : "Type: Float");
			}
			if (variable.instance_count > 0) {
				ImGui::Text("Instances: %d", variable.instance_count);
			}
			if (!variable.defining_rule.empty()) {
				ImGui::Text("Last Definition: %s", variable.defining_rule.c_str());
			}
		} else if (scene_generation_in_progress()) {
			drew_details = true;
			ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
			ImGui::TextUnformatted("Runtime values update after regeneration finishes.");
			ImGui::PopStyleColor();
		}
	}

	return drew_details;
}

void draw_hovered_editor_token_tooltip(const std::vector<std::string> &lines,
                                       const GrammarSymbolIndex &symbols,
                                       ImGuiID editor_id,
                                       const ImRect &editor_rect)
{
	EditorToken hovered_token;
	EditorRange hovered_range;
	if (!find_hovered_editor_token(lines, symbols, editor_id, editor_rect, &hovered_token, &hovered_range)) {
		return;
	}

	ImGui::BeginTooltip();
	ImGui::TextUnformatted(hovered_token.text.c_str());
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
	ImGui::TextUnformatted(label_for_editor_token_kind(hovered_token.kind).c_str());
	ImGui::PopStyleColor();
	draw_divider();
	ImGui::Text("Line %d, Column %d", hovered_range.line + 1, hovered_range.start_column + 1);
	if (draw_selected_identifier_details(hovered_token)) {
		draw_divider();
	}
	ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
	ImGui::TextWrapped("%s", hovered_token.tooltip.c_str());
	ImGui::PopTextWrapPos();
	ImGui::EndTooltip();
}

void draw_part_class_autocomplete(EditorWorkspaceSession *app_state,
                                  const std::vector<std::string> &lines,
                                  const GrammarSymbolIndex &symbols,
                                  const EditorTokenMatch &cursor_position,
                                  ImGuiID editor_id,
                                  const ImRect &editor_rect,
                                  bool editor_active,
                                  bool editor_hovered)
{
	if (app_state == nullptr) {
		return;
	}

	PartClassAutocomplete autocomplete =
		build_spatial_declaration_autocomplete(lines, cursor_position);
	if (!autocomplete.active) {
		autocomplete = build_shape_option_autocomplete(lines, cursor_position);
	}
	if (!autocomplete.active) {
		autocomplete = build_part_class_autocomplete(lines, cursor_position);
	}
	if (!autocomplete.active || autocomplete.suggestions.empty()) {
		return;
	}

	EditorOverlayLayout layout;
	if (!build_editor_overlay_layout(editor_id, editor_rect, &layout) ||
	    autocomplete.line < 0 ||
	    autocomplete.line >= static_cast<int>(lines.size())) {
		return;
	}

	const std::string &line = lines[static_cast<std::size_t>(autocomplete.line)];
	ImVec2 popup_position =
		editor_screen_position_for_column(layout,
		                                  line,
		                                  autocomplete.line,
		                                  autocomplete.replace_end_column,
		                                  &symbols);
	popup_position.y += layout.line_height + 6.0f;
	popup_position.x = std::max(popup_position.x, editor_rect.Min.x + 6.0f);

	const int visible_count =
		std::min(static_cast<int>(autocomplete.suggestions.size()), 8);
	ImGui::SetNextWindowPos(popup_position, ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(280.0f, visible_count * (layout.line_height + 6.0f) + 34.0f), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.98f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, color_from_hex(0xfffdfa));
	ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xd6ccd9, 0.9f));
	if (ImGui::Begin("##PartClassAutocomplete",
	                 nullptr,
	                 ImGuiWindowFlags_NoDecoration |
	                     ImGuiWindowFlags_NoMove |
	                     ImGuiWindowFlags_NoSavedSettings |
	                     ImGuiWindowFlags_NoFocusOnAppearing |
	                     ImGuiWindowFlags_NoNav)) {
		ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
		const std::string autocomplete_label =
			autocomplete.spatial_declaration_context
				? autocomplete.context_label
				: autocomplete.shape_option_context ? "Shape Option" : "Part Class";
		ImGui::Text("%s%s",
		            autocomplete_label.c_str(),
		            autocomplete.prefix.empty() ? "" : (": " + autocomplete.prefix).c_str());
		ImGui::PopStyleColor();
		draw_divider();
		for (int index = 0; index < visible_count; ++index) {
			const std::string &suggestion = autocomplete.suggestions[static_cast<std::size_t>(index)];
			if (ImGui::Selectable(suggestion.c_str(), false)) {
				apply_part_class_autocomplete(app_state, lines, autocomplete, suggestion);
			}
		}
		if (static_cast<int>(autocomplete.suggestions.size()) > visible_count) {
			ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
			ImGui::Text("+ %zu more", autocomplete.suggestions.size() - static_cast<std::size_t>(visible_count));
			ImGui::PopStyleColor();
		}
	}
	ImGui::End();
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
}

void draw_material_autocomplete(EditorWorkspaceSession *app_state,
                                const std::vector<std::string> &lines,
                                const GrammarSymbolIndex &symbols,
                                const EditorTokenMatch &cursor_position,
                                ImGuiID editor_id,
                                const ImRect &editor_rect)
{
	if (app_state == nullptr) {
		return;
	}

	const MaterialAutocomplete autocomplete =
		build_material_autocomplete(lines, cursor_position);
	if (!autocomplete.active || autocomplete.suggestions.empty()) {
		return;
	}

	EditorOverlayLayout layout;
	if (!build_editor_overlay_layout(editor_id, editor_rect, &layout) ||
	    autocomplete.line < 0 ||
	    autocomplete.line >= static_cast<int>(lines.size())) {
		return;
	}

	const std::string &line = lines[static_cast<std::size_t>(autocomplete.line)];
	ImVec2 popup_position =
		editor_screen_position_for_column(layout,
		                                  line,
		                                  autocomplete.line,
		                                  cursor_position.column,
		                                  &symbols);
	popup_position.y += layout.line_height + 6.0f;
	popup_position.x = std::max(popup_position.x, editor_rect.Min.x + 6.0f);

	const MaterialAutocompletePrefixState prefix_state =
		analyze_material_autocomplete_prefix(autocomplete.prefix);
	const std::vector<MaterialLexemeKind> ordered_kinds =
		ordered_material_autocomplete_kinds(prefix_state);
	const int visible_count =
		std::min(static_cast<int>(autocomplete.suggestions.size()), 10);
	ImGui::SetNextWindowPos(popup_position, ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(320.0f, (visible_count + 5) * (layout.line_height + 4.0f)), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.98f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, color_from_hex(0xfffdfa));
	ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xd6ccd9, 0.9f));
	if (ImGui::Begin("##MaterialAutocomplete",
	                 nullptr,
	                 ImGuiWindowFlags_NoDecoration |
	                     ImGuiWindowFlags_NoMove |
	                     ImGuiWindowFlags_NoSavedSettings |
	                     ImGuiWindowFlags_NoFocusOnAppearing |
	                     ImGuiWindowFlags_NoNav)) {
		ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
		if (autocomplete.prefix.empty()) {
			ImGui::TextUnformatted("Material");
		} else {
			ImGui::Text("Material: %s", autocomplete.prefix.c_str());
		}
		ImGui::PopStyleColor();
		draw_divider();

		int drawn_count = 0;
		bool drew_section = false;
		for (MaterialLexemeKind kind : ordered_kinds) {
			std::vector<const MaterialAutocompleteSuggestion *> group;
			for (const MaterialAutocompleteSuggestion &suggestion : autocomplete.suggestions) {
				if (suggestion.kind == kind) {
					group.push_back(&suggestion);
				}
			}
			if (group.empty()) {
				continue;
			}
			if (drew_section) {
				draw_divider();
			}
			ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
			ImGui::TextUnformatted(label_for_material_autocomplete_kind(kind));
			ImGui::PopStyleColor();
			drew_section = true;
			for (const MaterialAutocompleteSuggestion *suggestion : group) {
				if (suggestion == nullptr || drawn_count >= visible_count) {
					continue;
				}
				if (kind == MaterialLexemeKind::Color) {
					ImGui::ColorButton(("##MaterialSwatch" + suggestion->label).c_str(),
					                   ImVec4(suggestion->swatch.r,
					                          suggestion->swatch.g,
					                          suggestion->swatch.b,
					                          1.0f),
					                   ImGuiColorEditFlags_NoTooltip |
					                       ImGuiColorEditFlags_NoDragDrop,
					                   ImVec2(layout.line_height - 2.0f, layout.line_height - 2.0f));
					ImGui::SameLine();
				}
				if (ImGui::Selectable(suggestion->label.c_str(), false)) {
					apply_material_autocomplete(app_state, lines, autocomplete, *suggestion);
				}
				++drawn_count;
			}
			if (drawn_count >= visible_count) {
				break;
			}
		}
		if (static_cast<int>(autocomplete.suggestions.size()) > drawn_count) {
			ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b938c));
			ImGui::Text("+ %zu more",
			            autocomplete.suggestions.size() - static_cast<std::size_t>(drawn_count));
			ImGui::PopStyleColor();
		}
	}
	ImGui::End();
	ImGui::PopStyleColor(2);
	ImGui::PopStyleVar(2);
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

void request_editor_focus_restore()
{
	smart_editor_state.requested_cursor_column = smart_editor_state.cursor_column;
	smart_editor_state.requested_selection_start = smart_editor_state.selection_start;
	smart_editor_state.requested_selection_end = smart_editor_state.selection_end;
	smart_editor_state.request_cursor_sync = true;
	smart_editor_state.request_selection_sync = true;
	smart_editor_state.request_focus = true;
}

void commit_editor_text(EditorWorkspaceSession *app_state, const std::string &text)
{
	if (app_state == nullptr) {
		return;
	}
	app_state->document.replaceSourceText(text, true);
	ensure_scene_generation_services();
	const uint64_t design_nonce = ++next_grammar_design_nonce;
	scene_regeneration_coordinator->scheduleGeneration(app_state->document.source_text,
	                                                   0.0,
	                                                   design_nonce,
	                                                   false,
	                                                   glfwGetTime() + 2.0);
	smart_editor_state.synced_text = app_state->document.source_text;
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

int active_texture_slot_count(const TextureLibraryState &state)
{
	return static_cast<int>(std::count_if(state.entries.begin(),
	                                      state.entries.end(),
	                                      [](const BackendTextureSlot &entry) {
		                                      return entry.active;
	                                      }));
}

bool ai_result_has_grammar(const BackendAiResult &result)
{
	return !trim_copy(result.grammar).empty();
}

bool ai_proposal_matches_current_document(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr || !app_state->ai_assistant.proposal_review.isPending()) {
		return false;
	}
	if (app_state->ai_assistant.proposal_review.isBasedOnSource(app_state->document.source_text)) {
		return true;
	}
	set_ai_assistant_feedback(
		app_state,
		"The document changed after this AI proposal was created. Request a new proposal before accepting it.",
		true);
	return false;
}

void accept_full_ai_proposal(EditorWorkspaceSession *app_state)
{
	if (!ai_proposal_matches_current_document(app_state)) {
		return;
	}
	const std::optional<std::string> accepted_source =
		app_state->ai_assistant.proposal_review.acceptFullProposal();
	if (!accepted_source.has_value()) {
		return;
	}
	commit_editor_text(app_state, *accepted_source);
	request_editor_focus_restore();
	set_ai_assistant_feedback(app_state,
	                          "Accepted the full AI grammar proposal. Validation is scheduled.",
	                          false);
}

void accept_selected_ai_changes(EditorWorkspaceSession *app_state)
{
	if (!ai_proposal_matches_current_document(app_state)) {
		return;
	}
	const std::optional<std::string> accepted_source =
		app_state->ai_assistant.proposal_review.acceptSelectedChanges();
	if (!accepted_source.has_value()) {
		return;
	}
	commit_editor_text(app_state, *accepted_source);
	request_editor_focus_restore();
	set_ai_assistant_feedback(app_state,
	                          "Accepted the selected AI changes. Validation is scheduled.",
	                          false);
}

void reject_ai_proposal(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr || !app_state->ai_assistant.proposal_review.isPending()) {
		return;
	}
	app_state->ai_assistant.proposal_review.rejectProposal();
	set_ai_assistant_feedback(app_state, "Rejected the AI grammar proposal.", false);
}

void draw_ai_grammar_proposal_content(EditorWorkspaceSession &workspace_session)
{
	AiAssistantSession &state = workspace_session.ai_assistant;
	ImGui::Dummy(ImVec2(0.0f, 8.0f));
	ImGui::TextUnformatted("Proposal Review");
	if (state.proposal_review.isPending()) {
		const bool current_source_matches =
			state.proposal_review.isBasedOnSource(workspace_session.document.source_text);
		draw_status_chip(current_source_matches ? std::string("Ready for review")
		                                               : std::string("Source changed"),
		                 current_source_matches ? 0x7ea88e : 0xca7f7f,
		                 current_source_matches ? 0x527564 : 0x7f4848,
		                 0.12f,
		                 0.24f);
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
		ImGui::TextWrapped(
			"AI grammar is a proposal only. Select complete change groups, then explicitly accept or reject it.");
		ImGui::PopStyleColor();

		const std::vector<AiGrammarSourceDifference> &differences =
			state.proposal_review.differences();
		ImGui::PushStyleColor(ImGuiCol_ChildBg, color_from_hex(0xffffff));
		ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xd8e1ea));
		ImGui::BeginChild("AiGrammarDifferences", ImVec2(0.0f, 150.0f), true);
		if (differences.empty()) {
			ImGui::TextUnformatted("The proposed grammar matches the current source.");
		}
		for (std::size_t difference_index = 0;
		     difference_index < differences.size();
		     ++difference_index) {
			const AiGrammarSourceDifference &difference = differences[difference_index];
			ImGui::PushID(static_cast<int>(difference_index));
			bool selected = difference.selected;
			const std::string change_label =
				"Change at original line " + std::to_string(difference.original_start_line);
			if (ImGui::Checkbox(change_label.c_str(), &selected)) {
				state.proposal_review.setDifferenceSelected(difference_index, selected);
			}
			if (app_fonts.mono != nullptr) {
				ImGui::PushFont(app_fonts.mono);
			}
			for (const std::string &line : difference.original_lines) {
				ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9b4f4f));
				ImGui::TextWrapped("- %s", line.c_str());
				ImGui::PopStyleColor();
			}
			for (const std::string &line : difference.proposed_lines) {
				ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x3f7655));
				ImGui::TextWrapped("+ %s", line.c_str());
				ImGui::PopStyleColor();
			}
			if (app_fonts.mono != nullptr) {
				ImGui::PopFont();
			}
			ImGui::Separator();
			ImGui::PopID();
		}
		ImGui::EndChild();
		ImGui::PopStyleColor(2);

		if (ImGui::Button("Accept Full Proposal")) {
			accept_full_ai_proposal(&workspace_session);
		}
		ImGui::SameLine();
		if (ImGui::Button("Accept Selected Changes")) {
			accept_selected_ai_changes(&workspace_session);
		}
		ImGui::SameLine();
		if (ImGui::Button("Reject Proposal")) {
			reject_ai_proposal(&workspace_session);
		}
	} else if (state.proposal_review.decision() ==
	           AiGrammarProposalDecision::AcceptedFullProposal) {
		draw_status_chip("Full proposal accepted", 0x7ea88e, 0x527564, 0.12f, 0.24f);
	} else if (state.proposal_review.decision() ==
	           AiGrammarProposalDecision::AcceptedSelectedChanges) {
		draw_status_chip("Selected changes accepted", 0x7ea88e, 0x527564, 0.12f, 0.24f);
	} else if (state.proposal_review.decision() == AiGrammarProposalDecision::Rejected) {
		draw_status_chip("Proposal rejected", 0xca7f7f, 0x7f4848, 0.12f, 0.24f);
	}
}

void draw_ai_assistant_content(EditorWorkspaceSession &workspace_session)
{
	EditorWorkspaceSession *app_state = &workspace_session;
	draw_section_heading("AI Assistant");

	AiAssistantSession &state = app_state->ai_assistant;
	ImGui::SameLine();
	draw_status_chip(std::to_string(state.threads.size()) + " threads", 0x6f95ba, 0x436786);
	if (!trim_copy(state.active_model).empty()) {
		ImGui::SameLine();
		draw_status_chip(state.active_model, 0x93a9bc, 0x5a7187);
	} else if (!trim_copy(app_state->authentication.backend_user.preferences.ai_model).empty()) {
		ImGui::SameLine();
		draw_status_chip(app_state->authentication.backend_user.preferences.ai_model, 0x93a9bc, 0x5a7187);
	}
	ImGui::SameLine();
	if (ImGui::Button("Refresh Threads")) {
		refresh_ai_threads_internal(app_state);
	}
	ImGui::SameLine();
	if (ImGui::Button("New Thread")) {
		start_new_ai_thread(app_state);
	}

	if (!app_state->authentication.session.authenticated || trim_copy(app_state->authentication.config.backend_base_url).empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
		ImGui::TextWrapped("Sign in with a configured backend to use the server-side AI assistant.");
		ImGui::PopStyleColor();
		draw_divider();
		return;
	}

	if (!state.status_message.empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text,
		                      state.status_is_error ? color_from_hex(0xb85d5d) : color_from_hex(0x527564));
		ImGui::TextWrapped("%s", state.status_message.c_str());
		ImGui::PopStyleColor();
	}

	if (ImGui::BeginTable("AiAssistantLayout",
	                      2,
	                      ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadOuterX)) {
		ImGui::TableSetupColumn("AiThreads", ImGuiTableColumnFlags_WidthStretch, 0.82f);
		ImGui::TableSetupColumn("AiComposer", ImGuiTableColumnFlags_WidthStretch, 1.48f);

		ImGui::TableNextColumn();
		ImGui::BeginChild("AiThreadList", ImVec2(0.0f, 0.0f), true);
		if (!state.threads_loaded) {
			ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
			ImGui::TextUnformatted("AI threads will appear here after the first refresh.");
			ImGui::PopStyleColor();
		} else if (state.threads.empty()) {
			ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
			ImGui::TextUnformatted("No AI threads yet. Start with a new request.");
			ImGui::PopStyleColor();
		} else {
			for (const BackendAiThreadSummary &thread : state.threads) {
				ImGui::PushID(thread.id.c_str());
				const bool selected = state.selected_thread_id == thread.id;
				std::string label = thread.title.empty() ? ai_mode_label(thread.mode) : thread.title;
				if (ImGui::Selectable(label.c_str(), selected, ImGuiSelectableFlags_AllowItemOverlap)) {
					load_ai_thread(app_state, thread.id, true);
				}
				if (!thread.mode.empty()) {
					ImGui::SameLine();
					draw_status_chip(ai_mode_label(thread.mode), 0x8ea5ba, 0x556d83);
				}
				if (thread.message_count > 0) {
					ImGui::SameLine();
					draw_status_chip(std::to_string(thread.message_count) + " msgs", 0x93a9bc, 0x5a7187);
				}
				if (!thread.last_message_preview.empty()) {
					ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
					ImGui::TextWrapped("%s", thread.last_message_preview.c_str());
					ImGui::PopStyleColor();
				}
				if (!thread.updated_at.empty()) {
					ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x9aa7b2));
					ImGui::TextUnformatted(thread.updated_at.c_str());
					ImGui::PopStyleColor();
				}
				draw_divider();
				ImGui::PopID();
			}
		}
		ImGui::EndChild();

		ImGui::TableNextColumn();
		ImGui::BeginChild("AiComposerPane", ImVec2(0.0f, 0.0f), false);
		const char *modes[] = {
			"active_helper_chat",
			"draft_grammar",
			"repair_grammar",
			"explain_grammar",
			"tutor_next_step"};
		if (ImGui::BeginCombo("Mode", ai_mode_label(state.mode).c_str())) {
			for (const char *mode : modes) {
				const bool selected = state.mode == mode;
				if (ImGui::Selectable(ai_mode_label(mode).c_str(), selected)) {
					state.mode = mode;
				}
				if (selected) {
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
		ImGui::TextWrapped("%s", ai_mode_help_text(state.mode).c_str());
		ImGui::PopStyleColor();

		ImGui::Dummy(ImVec2(0.0f, 6.0f));
		if (!state.current_thread.id.empty()) {
			draw_status_chip(state.current_thread.title.empty() ? std::string("Current Thread") : state.current_thread.title,
			                 0x6f95ba,
			                 0x436786);
			if (!state.current_thread.file_title.empty()) {
				ImGui::SameLine();
				draw_status_chip(state.current_thread.file_title, 0x93a9bc, 0x5a7187);
			}
		} else {
			draw_status_chip("Unsaved Thread", 0xc8a766, 0x7a6035, 0.14f, 0.26f);
		}

		const std::string selection = selected_editor_text(app_state);
		if (!selection.empty()) {
			ImGui::SameLine();
			draw_status_chip("Selection attached", 0x7ea88e, 0x527564, 0.12f, 0.24f);
		}
		if (!app_state->document_diagnostics.empty()) {
			ImGui::SameLine();
			draw_status_chip(std::to_string(app_state->document_diagnostics.size()) + " parser issues",
			                 0xca7f7f,
			                 0x7f4848,
			                 0.12f,
			                 0.24f);
		}
		if (!app_state->storage_identity.cloud_file_id.empty()) {
			ImGui::SameLine();
			draw_status_chip("Cloud file context", 0x7ea88e, 0x527564, 0.12f, 0.24f);
		}

		ImGui::Dummy(ImVec2(0.0f, 6.0f));
		ImGui::TextUnformatted(ai_mode_prefers_prompt(state.mode) ? "Goal / Prompt" : "Question");
		input_text_multiline_string("##AiRequestInput", &state.request_input, ImVec2(-1.0f, 108.0f));

		if (ImGui::Button("Send Request")) {
			BackendAiGenerateRequest request;
			request.mode = state.mode;
			if (ai_mode_prefers_prompt(state.mode)) {
				request.prompt = state.request_input;
			} else {
				request.question = state.request_input;
			}
			request.grammar = app_state->document.source_text;
			request.selection = selection;
			request.parser_error = current_parser_error_summary(app_state);
			request.thread_id = state.selected_thread_id;
			request.file_id = app_state->storage_identity.cloud_file_id;
			request.title = default_cloud_title(app_state);
			request.history = ai_request_history(state);

			AiGrammarProposal proposal;
			std::string error;
			BackendAiGrammarProposalService proposal_service(app_state->authentication.config,
			                                                  app_state->authentication.session);
			if (proposal_service.submitProposal(request, &proposal, &error)) {
				persist_current_firebase_session(app_state);
				if (proposal.credits.loaded) {
					app_state->authentication.backend_user.credits = proposal.credits;
				}
				state.mode = proposal.mode.empty() ? state.mode : proposal.mode;
				state.current_thread = proposal.thread;
				state.selected_thread_id = proposal.thread.id;
				state.messages = proposal.conversation_messages;
				state.last_usage = proposal.usage;
				state.last_result = proposal.result;
				state.active_model = proposal.model_name;
				state.threads_loaded = true;
				if (!state.last_result.loaded) {
					sync_ai_result_from_messages(&state);
				}
				stage_ai_proposal_review(app_state);
				refresh_ai_threads_internal(app_state, true);
				set_ai_assistant_feedback(
					app_state,
					state.proposal_review.isPending()
						? "AI proposal received. Review its source differences before accepting it."
						: "AI response received.",
					false);
			} else {
				if (proposal.credits.loaded) {
					app_state->authentication.backend_user.credits = proposal.credits;
				}
				set_ai_assistant_feedback(app_state,
				                          error.empty() ? "Could not complete the AI request." : error,
				                          true);
			}
		}
		if (!state.current_thread.id.empty()) {
			ImGui::SameLine();
			if (ImGui::Button("Reload Thread")) {
				load_ai_thread(app_state, state.current_thread.id, true);
			}
		}

		if (state.last_usage.loaded) {
			ImGui::Dummy(ImVec2(0.0f, 6.0f));
			draw_status_chip(state.last_usage.status.empty() ? std::string("usage") : state.last_usage.status,
			                 0x93a9bc,
			                 0x5a7187);
			ImGui::SameLine();
			draw_status_chip(std::to_string(state.last_usage.final_credits) + " final credits",
			                 0x7ea88e,
			                 0x527564,
			                 0.12f,
			                 0.24f);
			ImGui::SameLine();
			draw_status_chip(std::to_string(state.last_usage.total_tokens) + " tokens",
			                 0x8ea5ba,
			                 0x556d83);
		}

		if (state.last_result.loaded) {
			ImGui::Dummy(ImVec2(0.0f, 8.0f));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, color_from_hex(0xf9fbfe));
			ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xd8e1ea));
			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
			ImGui::BeginChild("AiResultCard", ImVec2(0.0f, 460.0f), true);
			if (!state.last_result.title.empty()) {
				draw_status_chip(state.last_result.title, 0x6f95ba, 0x436786);
			}

			const auto draw_result_text = [&](const char *label, const std::string &value) {
				if (trim_copy(value).empty()) {
					return;
				}
				ImGui::Dummy(ImVec2(0.0f, 4.0f));
				ImGui::TextUnformatted(label);
				ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x334658));
				ImGui::TextWrapped("%s", value.c_str());
				ImGui::PopStyleColor();
			};
			const auto draw_result_list = [&](const char *label, const std::vector<std::string> &values) {
				if (values.empty()) {
					return;
				}
				ImGui::Dummy(ImVec2(0.0f, 4.0f));
				ImGui::TextUnformatted(label);
				for (const std::string &value : values) {
					ImGui::BulletText("%s", value.c_str());
				}
			};

			draw_result_text("Summary", state.last_result.summary);
			draw_result_text("Answer", state.last_result.answer);
			draw_result_text("Repair Summary", state.last_result.repair_summary);
			draw_result_text("Lesson", state.last_result.lesson);
			draw_result_text("Diagnosis", state.last_result.diagnosis);
			draw_result_text("Practice Prompt", state.last_result.practice_prompt);
			draw_result_list("Motifs", state.last_result.motifs);
			draw_result_list("Next Steps", state.last_result.next_steps);
			draw_result_list("Changes", state.last_result.changes);
			draw_result_list("Observations", state.last_result.observations);
			draw_result_list("Suggested Edits", state.last_result.suggested_edits);
			draw_result_list("Actions", state.last_result.actions);
			draw_result_list("Warnings", state.last_result.warnings);

			if (ai_result_has_grammar(state.last_result)) {
				ImGui::Dummy(ImVec2(0.0f, 4.0f));
				ImGui::TextUnformatted("Returned Grammar");
				ImGui::PushStyleColor(ImGuiCol_ChildBg, color_from_hex(0xffffff));
				ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xd8e1ea));
				ImGui::BeginChild("AiResultGrammar", ImVec2(0.0f, 96.0f), true);
				if (app_fonts.mono != nullptr) {
					ImGui::PushFont(app_fonts.mono);
				}
				ImGui::TextUnformatted(state.last_result.grammar.c_str());
				if (app_fonts.mono != nullptr) {
					ImGui::PopFont();
				}
				ImGui::EndChild();
				ImGui::PopStyleColor(2);

				ai_grammar_proposal_panel.draw(workspace_session);
			}

			ImGui::EndChild();
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(2);
		}

		ImGui::Dummy(ImVec2(0.0f, 8.0f));
		ImGui::TextUnformatted("Thread Messages");
		ImGui::PushStyleColor(ImGuiCol_ChildBg, color_from_hex(0xf9fbfe));
		ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xd8e1ea));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
		ImGui::BeginChild("AiMessageLog", ImVec2(0.0f, 0.0f), true);
		if (state.messages.empty()) {
			ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
			ImGui::TextUnformatted("No messages yet.");
			ImGui::PopStyleColor();
		} else {
			for (const BackendAiMessage &message : state.messages) {
				const bool assistant = lowercase_copy(message.role) == "assistant";
				draw_status_chip(assistant ? "assistant" : "user",
				                 assistant ? 0x7ea88e : 0x8ea5ba,
				                 assistant ? 0x527564 : 0x556d83,
				                 assistant ? 0.12f : 0.10f,
				                 0.24f);
				ImGui::SameLine();
				draw_status_chip(ai_mode_label(message.mode), 0x93a9bc, 0x5a7187);
				if (!message.created_at.empty()) {
					ImGui::SameLine();
					draw_status_chip(message.created_at, 0x9aa7b2, 0x667788);
				}
				ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x334658));
				ImGui::TextWrapped("%s", message.content.c_str());
				ImGui::PopStyleColor();
				draw_divider();
			}
		}
		ImGui::EndChild();
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(2);
		ImGui::EndChild();

		ImGui::EndTable();
	}

	draw_divider();
}

void draw_texture_library_controls(EditorWorkspaceSession *app_state)
{
	draw_section_heading("Texture Library");
	if (app_state == nullptr) {
		return;
	}

	TextureLibraryState &state = app_state->texture_library;
	ImGui::SameLine();
	draw_status_chip(std::to_string(active_texture_slot_count(state)) + "/20 active",
	                 0x7ea88e,
	                 0x527564,
	                 0.12f,
	                 0.24f);
	ImGui::SameLine();
	if (ImGui::Button("Refresh Library")) {
		refresh_texture_library_internal(app_state);
	}

	if (!app_state->authentication.session.authenticated || trim_copy(app_state->authentication.config.backend_base_url).empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
		ImGui::TextWrapped("Sign in with a configured backend to manage `usertexture1` through `usertexture20`.");
		ImGui::PopStyleColor();
		draw_divider();
		return;
	}

	if (!state.status_message.empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text,
		                      state.status_is_error ? color_from_hex(0xb85d5d) : color_from_hex(0x527564));
		ImGui::TextWrapped("%s", state.status_message.c_str());
		ImGui::PopStyleColor();
	}

	if (state.entries.empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
		ImGui::TextWrapped("The texture library has not loaded yet.");
		ImGui::PopStyleColor();
		draw_divider();
		return;
	}

	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
	ImGui::TextWrapped("Reference these slots directly in grammar materials, for example `I ( Cube usertexture1 ... )`.");
	ImGui::PopStyleColor();
	draw_divider();

	if (ImGui::BeginTable("TextureLibraryLayout",
	                      2,
	                      ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadOuterX)) {
		ImGui::TableSetupColumn("TextureSlots", ImGuiTableColumnFlags_WidthStretch, 0.84f);
		ImGui::TableSetupColumn("TextureDetail", ImGuiTableColumnFlags_WidthStretch, 1.46f);

		ImGui::TableNextColumn();
		ImGui::BeginChild("TextureSlotList", ImVec2(0.0f, 360.0f), true);
		for (const BackendTextureSlot &entry : state.entries) {
			const bool selected = state.selected_slot == entry.slot;
			std::string label = entry.slot + "  ";
			label += entry.active ? entry.display_name : std::string("Empty");
			if (ImGui::Selectable(label.c_str(), selected)) {
				state.selected_slot = entry.slot;
				sync_texture_library_inputs_from_selection(app_state);
			}
			if (entry.active) {
				ImGui::SameLine();
				draw_status_chip(entry.source.empty() ? "stored" : entry.source, 0x93a9bc, 0x5a7187);
			}
		}
		ImGui::EndChild();

		ImGui::TableNextColumn();
		BackendTextureSlot *entry = selected_texture_library_entry(app_state);
		if (entry != nullptr) {
			draw_status_chip(entry->slot, 0x6f95ba, 0x436786);
			ImGui::SameLine();
			draw_status_chip(entry->active ? "Active" : "Empty",
			                 entry->active ? 0x7ea88e : 0xc8a766,
			                 entry->active ? 0x527564 : 0x7a6035,
			                 0.12f,
			                 0.24f);
			if (!entry->source.empty()) {
				ImGui::SameLine();
				draw_status_chip(entry->source, 0x93a9bc, 0x5a7187);
			}
			if (!entry->updated_at.empty()) {
				ImGui::SameLine();
				draw_status_chip(entry->updated_at, 0x8ea5ba, 0x556d83);
			}

			ImGui::Dummy(ImVec2(0.0f, 8.0f));
			if (state.preview_texture != 0 && state.preview_slot == entry->slot) {
				ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(state.preview_texture)),
				             ImVec2(160.0f, 160.0f),
				             ImVec2(0.0f, 1.0f),
				             ImVec2(1.0f, 0.0f));
			} else {
				ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
				ImGui::TextUnformatted(entry->active ? "Preview will load when the slot is selected." : "No texture uploaded for this slot.");
				ImGui::PopStyleColor();
			}

			ImGui::Dummy(ImVec2(0.0f, 8.0f));
			ImGui::TextUnformatted("Display Name");
			input_text_string("##TextureDisplayName", &state.display_name_input);
			ImGui::TextUnformatted("Alpha");
			ImGui::SliderFloat("##TextureAlpha", &state.alpha_value, 0.0f, 1.0f, "%.2f");

			if (ImGui::Button("Save Metadata")) {
				std::vector<BackendTextureSlot> entries;
				std::string error;
				BackendTextureLibraryRepository texture_repository(app_state->authentication.config,
				                                                   app_state->authentication.session);
				if (texture_repository.updateSlot(entry->slot,
				                                  state.display_name_input,
				                                  state.alpha_value,
				                                  &entries,
				                                  &error)) {
					apply_texture_library_entries(app_state, entries, true);
					set_texture_library_feedback(app_state, "Updated texture metadata.", false);
				} else {
					set_texture_library_feedback(app_state, error.empty() ? "Could not update the texture metadata." : error, true);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Delete Texture")) {
				std::vector<BackendTextureSlot> entries;
				std::string error;
				BackendTextureLibraryRepository texture_repository(app_state->authentication.config,
				                                                   app_state->authentication.session);
				if (texture_repository.deleteSlot(entry->slot, &entries, &error)) {
					apply_texture_library_entries(app_state, entries, true);
					set_texture_library_feedback(app_state, "Deleted the texture slot.", false);
				} else {
					set_texture_library_feedback(app_state, error.empty() ? "Could not delete the texture slot." : error, true);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Reload Preview")) {
				load_selected_texture_preview(app_state);
			}

			draw_divider();
			ImGui::TextUnformatted("Upload Path");
			input_text_string("##TextureUploadPath", &state.upload_path_input);
			if (ImGui::Button("Upload Texture File")) {
				std::vector<BackendTextureSlot> entries;
				std::string error;
				BackendTextureLibraryRepository texture_repository(app_state->authentication.config,
				                                                   app_state->authentication.session);
				if (texture_repository.uploadSlot(entry->slot,
				                                  state.display_name_input,
				                                  state.alpha_value,
				                                  state.upload_path_input,
				                                  &entries,
				                                  &error)) {
					apply_texture_library_entries(app_state, entries, true);
					set_texture_library_feedback(app_state, "Uploaded texture file.", false);
				} else {
					set_texture_library_feedback(app_state, error.empty() ? "Could not upload the texture file." : error, true);
				}
			}

			draw_divider();
			ImGui::TextUnformatted("Generate Prompt");
			input_text_multiline_string("##TexturePrompt", &state.prompt_input, ImVec2(-1.0f, 88.0f));
			if (ImGui::Button("Generate Texture")) {
				std::vector<BackendTextureSlot> entries;
				BackendCreditSummary credits;
				std::string error;
				BackendTextureLibraryRepository texture_repository(app_state->authentication.config,
				                                                   app_state->authentication.session);
				if (texture_repository.generateSlot(entry->slot,
				                                    state.display_name_input,
				                                    state.alpha_value,
				                                    state.prompt_input,
				                                    &entries,
				                                    &credits,
				                                    &error)) {
					if (credits.loaded) {
						app_state->authentication.backend_user.credits = credits;
					}
					apply_texture_library_entries(app_state, entries, true);
					set_texture_library_feedback(app_state, "Generated texture and refreshed credits.", false);
				} else {
					set_texture_library_feedback(app_state, error.empty() ? "Could not generate the texture." : error, true);
				}
			}
		}

		ImGui::EndTable();
	}

	draw_divider();
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

	const GLuint texture = load_texture_from_file(filename);
	if (texture == 0) {
		return false;
	}

	destroy_ui_image(image);
	image->texture = texture;
	image->width = width;
	image->height = height;
	return true;
}

bool regenerate_scene(const std::string &source_text)
{
	request_scene_regeneration_internal(source_text);
	return true;
}

void clear_auto_run_timer(EditorWorkspaceSession *app_state)
{
	(void)app_state;
	if (scene_regeneration_coordinator != nullptr) {
		scene_regeneration_coordinator->cancelScheduledRequest();
	}
}

void process_auto_run_timer(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr || scene_regeneration_coordinator == nullptr ||
	    !scene_generation_scheduled()) {
		return;
	}

	const double now = glfwGetTime();
	if (!scene_regeneration_coordinator->startScheduledRequestIfDue(now)) {
		return;
	}

	smart_editor_state.preserve_focus_through_regeneration = true;
	request_editor_focus_restore();
}

void export_ply(const std::string &path)
{
	const SceneGenerationContext *scene_context = current_scene_snapshot() != nullptr
		? current_scene_snapshot()->generationContext()
		: nullptr;
	if (scene_context == nullptr) {
		errorout("No last valid scene is available to export");
		return;
	}

	const auto base_vertices = build_base_vertex_data();
	MeshExportService export_service;
	std::string error;
	if (!export_service.exportScene(path,
	                               *scene_context,
	                               base_vertices.data(),
	                               default_ply_mesh_writer(),
	                               &error)) {
		errorout(error);
		return;
	}
	debugout("Exported PLY: " + path);
}

void draw_material_metric_bar(const char *label, float value, unsigned int fill_rgb)
{
	const float clamped_value = std::clamp(value, 0.0f, 1.0f);
	ImGui::BeginGroup();
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x667788));
	ImGui::TextUnformatted(label);
	ImGui::PopStyleColor();
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x223447));
	ImGui::Text("%.2f", clamped_value);
	ImGui::PopStyleColor();
	ImVec2 bar_size(ImGui::GetContentRegionAvail().x, 7.0f);
	bar_size.x = std::max(bar_size.x, 72.0f);
	ImGui::InvisibleButton((std::string("##") + label).c_str(), bar_size);
	const ImRect rect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	ImDrawList *draw_list = ImGui::GetWindowDrawList();
	draw_list->AddRectFilled(rect.Min,
	                         rect.Max,
	                         ImGui::GetColorU32(color_from_hex(0xecf2f7)),
	                         5.0f);
	const ImVec2 fill_max(rect.Min.x + rect.GetWidth() * clamped_value, rect.Max.y);
	draw_list->AddRectFilled(rect.Min,
	                         fill_max,
	                         ImGui::GetColorU32(color_from_hex(fill_rgb)),
	                         5.0f);
	draw_list->AddRect(rect.Min,
	                   rect.Max,
	                   ImGui::GetColorU32(color_from_hex(0xd7e0e8)),
	                   5.0f);
	ImGui::EndGroup();
}

void draw_material_library_content(EditorWorkspaceSession &workspace_session)
{
	EditorWorkspaceSession *app_state = &workspace_session;
	draw_section_heading("Materials");
	ImGui::SameLine();
	draw_status_chip(std::to_string(active_materials.size()) + " generated", 0x6f95ba, 0x436786);
	if (app_fonts.small != nullptr) {
		ImGui::PushFont(app_fonts.small);
	}
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
	ImGui::TextWrapped("Materials are inferred from names inside `I(...)`. Combine color, opacity, finish, pattern, and family words such as `rosefloral`, `oakverticalboards`, `walnuttwotoneboards`, `veinedmarble`, or `activeglowingneon`.");
	ImGui::PopStyleColor();
	draw_divider();

	ImGui::BulletText("Color words: white, blue, red, green, silver, gold, copper, grey, beige, magenta, lime, violet, chrome, amber.");
	ImGui::BulletText("Opacity words: opaque, translucent, transparent, smoky.");
	ImGui::BulletText("Finish and pattern words: shiny, glossy, polished, matte, rough, smooth, emissive, glowing, mirror, twotone, horizontal, vertical, veins, rotating, active.");
	ImGui::BulletText("Families: metal, glass, wood, boards, floral, marble, stone, concrete, ceramic, plastic, plaster, grass, brick, neon.");
	ImGui::BulletText("Wood variants: oak, walnut, pine, cedar, birch.");
	draw_divider();
	if (app_fonts.small != nullptr) {
		ImGui::PopFont();
	}

	draw_texture_library_controls(app_state);

	if (active_materials.empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
		ImGui::TextUnformatted("No generated materials yet. Regenerate a grammar containing `I ( Cube materialname ... )`.");
		ImGui::PopStyleColor();
		return;
	}

	const auto badge_color_for_family = [](const std::string &family) {
		if (family == "metal") return 0xc9a86a;
		if (family == "glass") return 0x84b6d4;
		if (family == "wood") return 0xb48358;
		if (family == "boards") return 0xbd8a5b;
		if (family == "floral") return 0xd487ab;
		if (family == "grass") return 0x74b15d;
		if (family == "brick") return 0xc27a62;
		if (family == "neon") return 0xae6ce5;
		if (family == "stone" || family == "concrete") return 0x9aa0a7;
		if (family == "marble") return 0xb9b5ca;
		if (family == "ceramic") return 0x88af9a;
		if (family == "plastic") return 0xd48ea8;
		return 0xb8a895;
	};

	const float available_width = ImGui::GetContentRegionAvail().x;
	const int column_count = available_width >= 880.0f ? 2 : 1;
	if (ImGui::BeginTable("MaterialsGrid", column_count, ImGuiTableFlags_SizingStretchSame)) {
		for (std::size_t i = 0; i < active_materials.size(); ++i) {
			const PreviewMaterial &material = active_materials[i];
			ImGui::TableNextColumn();
			ImGui::PushID(static_cast<int>(i));
			ImGui::PushStyleColor(ImGuiCol_ChildBg, color_from_hex(0xf9fbfe));
			ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xd8e1ea));
			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 12.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 11.0f));
			ImGui::BeginChild("MaterialCard", ImVec2(0.0f, 170.0f), true);
			ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(material.texture)),
			             ImVec2(72.0f, 72.0f),
			             ImVec2(0.0f, 1.0f),
			             ImVec2(1.0f, 0.0f));
			ImGui::SameLine(0.0f, 12.0f);
			ImGui::BeginGroup();
			if (app_fonts.section != nullptr) {
				ImGui::PushFont(app_fonts.section);
			}
			ImGui::TextUnformatted(material.name.c_str());
			if (app_fonts.section != nullptr) {
				ImGui::PopFont();
			}
			draw_status_chip(material.family.empty() ? "material" : material.family.c_str(),
			                 badge_color_for_family(material.family),
			                 badge_color_for_family(material.family),
			                 0.12f,
			                 0.24f);
			if (app_fonts.small != nullptr) {
				ImGui::PushFont(app_fonts.small);
			}
			ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x667788));
			ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 236.0f);
			ImGui::TextWrapped("%s", material.description.c_str());
			ImGui::PopTextWrapPos();
			ImGui::PopStyleColor();
			if (app_fonts.small != nullptr) {
				ImGui::PopFont();
			}
			ImGui::EndGroup();

			draw_divider();
			ImGui::TextUnformatted("Palette");
			for (std::size_t swatch_index = 0; swatch_index < material.swatches.size(); ++swatch_index) {
				const glm::vec3 &swatch = material.swatches[swatch_index];
				ImGui::PushID(static_cast<int>(swatch_index));
				if (swatch_index > 0) {
					ImGui::SameLine(0.0f, 8.0f);
				}
				ImGui::ColorButton("##MaterialSwatch",
				                   ImVec4(swatch.r, swatch.g, swatch.b, 1.0f),
				                   ImGuiColorEditFlags_NoTooltip |
				                       ImGuiColorEditFlags_NoDragDrop,
				                   ImVec2(22.0f, 22.0f));
				ImGui::PopID();
			}

			draw_divider();
			draw_material_metric_bar("Opacity", material.opacity, 0x8fc8d6);
			draw_material_metric_bar("Reflectance", material.reflectance, 0xc9a86a);
			draw_material_metric_bar("Smoothness", material.smoothness, 0xd49cb0);
			draw_material_metric_bar("Height", std::clamp(material.height_scale / 0.16f, 0.0f, 1.0f), 0x7aa6d8);
			draw_material_metric_bar("Glow", std::clamp(material.emission_strength / 2.5f, 0.0f, 1.0f), 0xf08ab8);

			ImGui::EndChild();
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(2);
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
}

}

void RenderSettingsPanel::draw(EditorWorkspaceSession &workspace_session) const
{
	EditorWorkspaceSession *app_state = &workspace_session;
	RenderConfiguration &configuration = app_state->preview.render_configuration;
	draw_section_heading("Render Settings");
	ImGui::SameLine();
	draw_status_chip("Quality & Effects", 0x7b93ae, 0x516a83);
	draw_divider();

	if (app_fonts.small != nullptr) {
		ImGui::PushFont(app_fonts.small);
	}
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
	ImGui::TextWrapped("Configure rendering quality, environmental effects, and post-processing options to enhance the visual quality of your procedural 3D models.");
	ImGui::PopStyleColor();
	draw_divider();
	if (app_fonts.small != nullptr) {
		ImGui::PopFont();
	}

	// Anti-Aliasing Section
		if (ImGui::CollapsingHeader("Anti-Aliasing", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x667788));
		ImGui::TextUnformatted("Anti-aliasing smooths jagged edges for improved visual quality.");
		ImGui::PopStyleColor();

		const char* aa_options[] = { "Off", "FXAA", "MSAA 2x", "MSAA 4x", "MSAA 8x" };
		const char* aa_descriptions[] = {
			"No anti-aliasing (fastest)",
			"Fast Approximate Anti-Aliasing (good performance)",
			"Multi-Sample Anti-Aliasing 2x (balanced)",
			"Multi-Sample Anti-Aliasing 4x (quality)",
			"Multi-Sample Anti-Aliasing 8x (best quality, slowest)"
		};

		ImGui::TextUnformatted("Method:");
		ImGui::SetNextItemWidth(200.0f);
		int current_aa = configuration.anti_aliasing_level;
		if (ImGui::Combo("##AntiAliasing", &current_aa, aa_options, 5)) {
			configuration.anti_aliasing_level = current_aa;
			configuration.changed = true;
		}

		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
		ImGui::TextWrapped("%s", aa_descriptions[current_aa]);
		ImGui::PopStyleColor();
			draw_divider();
		}

		if (ImGui::CollapsingHeader("Material Quality", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x667788));
			ImGui::TextWrapped("Procedural materials use physically based map sets with mipmaps and anisotropic filtering.");
			ImGui::PopStyleColor();

			const char *resolution_options[] = {"256", "512", "1024", "2048"};
			int resolution_index = 1;
			if (configuration.material_texture_resolution == 256) resolution_index = 0;
			else if (configuration.material_texture_resolution == 1024) resolution_index = 2;
			else if (configuration.material_texture_resolution == 2048) resolution_index = 3;
			ImGui::SetNextItemWidth(180.0f);
			if (ImGui::Combo("Texture resolution", &resolution_index,
			                 resolution_options, 4)) {
				static const int resolutions[] = {256, 512, 1024, 2048};
				configuration.material_texture_resolution = resolutions[resolution_index];
				configuration.changed = true;
				destroy_materials();
				request_scene_regeneration_internal(app_state->document.source_text);
			}
			ImGui::TextWrapped("512 is the balanced default. 1024 and 2048 improve close-up evidence captures but increase generation time and VRAM use.");
			draw_divider();
		}

		// Shadows Section
	if (ImGui::CollapsingHeader("Shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x667788));
		ImGui::TextUnformatted("Enable real-time shadow mapping for realistic lighting.");
		ImGui::PopStyleColor();

		if (ImGui::Checkbox("Enable Shadows", &configuration.shadows_enabled)) {
			configuration.changed = true;
		}
		draw_divider();
	}

	// Lens Flare Section
	if (ImGui::CollapsingHeader("Lens Flare", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x667788));
		ImGui::TextUnformatted("Add cinematic lens flare effects from bright light sources.");
		ImGui::PopStyleColor();

		if (ImGui::Checkbox("Enable Lens Flare", &configuration.lens_flare_enabled)) {
			configuration.changed = true;
		}
		draw_divider();
	}

	// Bloom Section
	if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x667788));
		ImGui::TextUnformatted("Bloom creates a soft glow effect around bright areas.");
		ImGui::PopStyleColor();

		if (ImGui::Checkbox("Enable Bloom", &configuration.bloom_enabled)) {
			configuration.changed = true;
		}

		if (configuration.bloom_enabled) {
			ImGui::TextUnformatted("Threshold:");
			ImGui::SetNextItemWidth(200.0f);
			if (ImGui::SliderFloat("##BloomThreshold", &configuration.bloom_threshold, 0.1f, 3.0f, "%.2f")) {
				configuration.changed = true;
			}

			ImGui::TextUnformatted("Intensity:");
			ImGui::SetNextItemWidth(200.0f);
			if (ImGui::SliderFloat("##BloomIntensity", &configuration.bloom_intensity, 0.1f, 2.0f, "%.2f")) {
				configuration.changed = true;
			}
		}
		draw_divider();
	}

	// Ambient Occlusion Section
	if (ImGui::CollapsingHeader("Ambient Occlusion", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x667788));
		ImGui::TextUnformatted("Screen-space ambient occlusion adds realistic contact shadows.");
		ImGui::PopStyleColor();

		if (ImGui::Checkbox("Enable SSAO", &configuration.ambient_occlusion_enabled)) {
			configuration.changed = true;
		}
		draw_divider();
	}

	// Environment Mapping Section
	if (ImGui::CollapsingHeader("Environment Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x667788));
		ImGui::TextUnformatted("Cubemap environment textures provide realistic reflections and ambient lighting.");
		ImGui::PopStyleColor();

		if (ImGui::Checkbox("Enable Environment Mapping", &configuration.cubemap_enabled)) {
			configuration.changed = true;
		}

		if (configuration.cubemap_enabled) {
			ImGui::TextUnformatted("Cubemap Resolution:");
			const char* resolution_options[] = { "256x256", "512x512", "1024x1024", "2048x2048" };
			int resolution_index = 0;
			if (configuration.cubemap_resolution == 256) resolution_index = 0;
			else if (configuration.cubemap_resolution == 512) resolution_index = 1;
			else if (configuration.cubemap_resolution == 1024) resolution_index = 2;
			else if (configuration.cubemap_resolution == 2048) resolution_index = 3;

			ImGui::SameLine();
			ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
			ImGui::TextUnformatted("(Recommended: 512)");
			ImGui::PopStyleColor();

			ImGui::SetNextItemWidth(150.0f);
			if (ImGui::Combo("##CubemapResolution", &resolution_index, resolution_options, 4)) {
				switch (resolution_index) {
					case 0: configuration.cubemap_resolution = 256; break;
					case 1: configuration.cubemap_resolution = 512; break;
					case 2: configuration.cubemap_resolution = 1024; break;
					case 3: configuration.cubemap_resolution = 2048; break;
				}
				configuration.changed = true;
			}

			ImGui::TextUnformatted("Intensity:");
			ImGui::SetNextItemWidth(200.0f);
			if (ImGui::SliderFloat("##CubemapIntensity", &configuration.cubemap_intensity, 0.1f, 3.0f, "%.2f")) {
				configuration.changed = true;
			}

			ImGui::Spacing();
			push_primary_action_button_style();
				push_primary_action_button_style();
				static bool cubemap_generation_success = true;
				static std::string cubemap_status_message = "";

				if (ImGui::Button("Generate Procedural Cubemap")) {
					// Delete old cubemap texture if it exists
					if (configuration.cubemap_texture != 0) {
						glDeleteTextures(1, &configuration.cubemap_texture);
						configuration.cubemap_texture = 0;
					}

					// Clear any previous OpenGL errors
					while (glGetError() != GL_NO_ERROR) {}

					// Generate new procedural cubemap
					configuration.cubemap_texture = generate_enhanced_procedural_cubemap(
						configuration.cubemap_resolution,
						configuration.cubemap_intensity);

					if (configuration.cubemap_texture != 0) {
						cubemap_generation_success = true;
						cubemap_status_message = "✓ Generated " + std::to_string(configuration.cubemap_resolution) + "x" +
							                         std::to_string(configuration.cubemap_resolution) + " cubemap";
						debugout(cubemap_status_message);
					} else {
						cubemap_generation_success = false;
						cubemap_status_message = "✗ Failed: Check console for errors";
						debugout("Failed to generate procedural cubemap");
					}
				}
				pop_primary_action_button_style();

				// Show generation status
				if (!cubemap_status_message.empty()) {
					ImGui::SameLine();
					if (cubemap_generation_success) {
						ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x5a8f5a));
					} else {
						ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0xc85a5a));
					}
					ImGui::TextUnformatted(cubemap_status_message.c_str());
					ImGui::PopStyleColor();
				}
			pop_primary_action_button_style();

			ImGui::Spacing();
			ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
			ImGui::TextWrapped("Generate a procedural environment texture with gradient sky, simulated sun, and atmospheric effects for realistic reflections.");
			ImGui::PopStyleColor();

			if (configuration.cubemap_texture != 0) {
				ImGui::Spacing();
				ImGui::TextUnformatted("Current Environment Map:");
				ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(configuration.cubemap_texture)),
				             ImVec2(128.0f, 64.0f),
				             ImVec2(0.0f, 1.0f),
				             ImVec2(1.0f, 0.0f));
			}
		}
		draw_divider();
	}

	// Tone Mapping Section
	if (ImGui::CollapsingHeader("Tone Mapping", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x667788));
		ImGui::TextUnformatted("Adjust exposure and gamma for proper HDR to LDR conversion.");
		ImGui::PopStyleColor();

		ImGui::TextUnformatted("Exposure:");
		ImGui::SetNextItemWidth(200.0f);
		if (ImGui::SliderFloat("##Exposure", &configuration.exposure, 0.1f, 5.0f, "%.2f")) {
			configuration.changed = true;
		}

		ImGui::TextUnformatted("Gamma:");
		ImGui::SetNextItemWidth(200.0f);
		if (ImGui::SliderFloat("##Gamma", &configuration.gamma, 1.0f, 3.0f, "%.2f")) {
			configuration.changed = true;
		}
		draw_divider();
	}

	// Performance Stats
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x667788));
	ImGui::TextUnformatted("Performance Note:");
	ImGui::PopStyleColor();
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
	ImGui::TextWrapped("Higher quality settings like MSAA 8x, high-resolution cubemaps, and SSAO may impact performance. Adjust based on your hardware capabilities.");
	ImGui::PopStyleColor();
}

namespace {

void draw_console()
{
	console_panel.draw(application_log, app_fonts.mono);
}

}

void ScenePreviewPanel::draw(EditorWorkspaceSession &workspace_session,
	                         float delta_seconds) const
{
	EditorWorkspaceSession *app_state = &workspace_session;
	RenderConfiguration &render_configuration = app_state->preview.render_configuration;

	// Check if render settings changed and trigger immediate preview update
	// This ensures all render settings apply immediately without requiring regeneration
	static int last_frame_settings_hash = 0;
	int current_settings_hash = 0;

	// Create a hash of all render settings to detect changes
	current_settings_hash ^= static_cast<int>(render_configuration.anti_aliasing_level) * 0x100;
	current_settings_hash ^= static_cast<int>(render_configuration.shadows_enabled) * 0x200;
	current_settings_hash ^= static_cast<int>(render_configuration.bloom_enabled) * 0x400;
	current_settings_hash ^= static_cast<int>(render_configuration.lens_flare_enabled) * 0x800;
	current_settings_hash ^= static_cast<int>(render_configuration.ambient_occlusion_enabled) * 0x1000;
	current_settings_hash ^= static_cast<int>(render_configuration.cubemap_enabled) * 0x2000;
	current_settings_hash ^=
		(render_configuration.material_texture_resolution / 256) * 0x4000;
	current_settings_hash ^= static_cast<int>(render_configuration.exposure * 10.0f);
	current_settings_hash ^= static_cast<int>(render_configuration.gamma * 10.0f);

	// Check if any render setting changed
	if (current_settings_hash != last_frame_settings_hash || render_configuration.changed) {
		// Force preview refresh with new settings
		render_configuration.changed = false;
		last_frame_settings_hash = current_settings_hash;

		// Mark that preview needs update with new settings
		// The render_scene_to_preview function will use the new settings immediately
		// No regeneration needed - render settings apply to existing geometry
	}

	const bool grammar_time_driven = grammar != nullptr && grammarUsesTime(grammar);
	const PreviewTimelineAdvance timeline_advance = preview_timeline_controller.advance(
		current_preview_session().timeline,
		delta_seconds,
		grammar_time_driven,
		scene_generation_in_progress());
	if (timeline_advance.stop_playback) {
		errorout("Grammar animation stopped because t exceeded the finite float range.");
	}
	if (timeline_advance.request_grammar_sample) {
		request_scene_time_sample_internal(app_state->document.source_text,
		                                   timeline_advance.grammar_sample_time);
	}
	if (timeline_advance.physics_delta_seconds > 0.0f) {
		step_preview_simulation(timeline_advance.physics_delta_seconds);
	}

	Context *context = grammar != nullptr ? grammar->context : nullptr;
	sanitize_preview_selection(app_state);
	const float current_simulation_time = grammar_time_driven
		? static_cast<float>(getGrammarEvaluationTime(grammar))
		: (context != nullptr ? context->getTime() : 0.0f);
	const float preview_material_time = static_cast<float>(glfwGetTime());
	const bool request_fit_camera = preview_timeline_panel.draw(
		workspace_session, context, grammar_time_driven, current_simulation_time);
	draw_divider();

	ImVec2 available = ImGui::GetContentRegionAvail();
	available.x = std::max(available.x, 100.0f);
	available.y = std::max(available.y, 100.0f);
	if (request_fit_camera || current_preview_session().fit_camera_pending) {
		fit_preview_camera_to_scene(app_state, available);
	}
	const PreviewInteractionCameraState camera =
		build_preview_camera_state(ImVec2(available.x, available.y));

	std::vector<PreviewOverlayVertex> overlay_vertices;
	if (context != nullptr) {
		if (connection_overlay_visible()) {
			overlay_vertices = build_connection_overlay_vertices(context);
		}
		append_axial_profile_overlay_vertices(&overlay_vertices, context);
		append_spatial_overlay_vertices(&overlay_vertices, context);
		append_collision_particle_overlay_vertices(&overlay_vertices, context);
	}
	append_light_gizmo_overlay_vertices(&overlay_vertices, camera);
			// Clear any pending OpenGL errors before upload_overlay_lines
			while (glGetError() != GL_NO_ERROR) {}
	upload_overlay_lines(overlay_vertices);
	upload_outline_batches(build_preview_outline_batches(context, app_state));

	const auto render_started_at = std::chrono::steady_clock::now();
	render_scene_to_preview(static_cast<int>(available.x),
	                        static_cast<int>(available.y),
	                        active_materials,
	                        current_preview_session().lighting.lights,
	                        preview_playback_active() ? preview_simulation_interpolation_alpha() : 1.0f,
	                        preview_material_time,
	                        preview_scale_factor(),
		                        preview_camera_distance_value(),
		                        preview_azimuth_degrees(),
		                        preview_elevation_degrees(),
		                        preview_roll_degrees(),
		                        preview_target_x_coordinate(),
	                        preview_target_y_coordinate(),
	                        preview_target_z_coordinate(),
	                        current_preview_session().projection.verticalFieldOfViewRadians(),
	                        preview_texture_debug_view,
								preview_mapping_mode_override,
										        render_configuration.lens_flare_enabled,
										        render_configuration.shadows_enabled,
										        render_configuration.anti_aliasing_level,
										        render_configuration.cubemap_enabled,
										        render_configuration.cubemap_texture,
										        render_configuration.cubemap_intensity,
										        render_configuration.bloom_enabled,
										        render_configuration.bloom_threshold,
										        render_configuration.bloom_intensity,
										        render_configuration.ambient_occlusion_enabled,
										        render_configuration.exposure,
										        render_configuration.gamma);
	current_preview_session().statistics.render_milliseconds =
		std::chrono::duration<double, std::milli>(
			std::chrono::steady_clock::now() - render_started_at)
			.count();

	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xe0d8d1));
	const PreviewTexturePresentation texture_presentation;
	const PreviewTextureCoordinate upper_left_texture_coordinate =
		texture_presentation.upperLeftTextureCoordinate();
	const PreviewTextureCoordinate lower_right_texture_coordinate =
		texture_presentation.lowerRightTextureCoordinate();
	ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(preview_texture_id())),
	             available,
	             ImVec2(upper_left_texture_coordinate.horizontal,
	                    upper_left_texture_coordinate.vertical),
	             ImVec2(lower_right_texture_coordinate.horizontal,
	                    lower_right_texture_coordinate.vertical));
	ImGui::PopStyleColor();
	ImGui::PopStyleVar();
	const ImRect preview_rect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	bool capture_pointer_from_view_cube = false;
	if (draw_preview_view_cube(preview_rect, &capture_pointer_from_view_cube)) {
		current_preview_session().fit_camera_pending = false;
	}
	const bool preview_hovered = ImGui::IsItemHovered();
	if (preview_hovered && !capture_pointer_from_view_cube) {
		ImGuiIO &io = ImGui::GetIO();
		PreviewInteractionSettings &interaction_settings =
			current_preview_session().interaction_settings;
		if (!io.WantTextInput) {
			PreviewNavigationInput navigation_input;
			navigation_input.move_left = ImGui::IsKeyDown(ImGuiKey_A);
			navigation_input.move_right = ImGui::IsKeyDown(ImGuiKey_D);
			navigation_input.move_forward = ImGui::IsKeyDown(ImGuiKey_W);
			navigation_input.move_backward = ImGui::IsKeyDown(ImGuiKey_S);
			navigation_input.move_down = ImGui::IsKeyDown(ImGuiKey_Q);
			navigation_input.move_up = ImGui::IsKeyDown(ImGuiKey_E);
			navigation_input.rotate_left = ImGui::IsKeyDown(ImGuiKey_Z);
			navigation_input.rotate_right = ImGui::IsKeyDown(ImGuiKey_C);
			preview_camera_motion_controller.advance(
				current_preview_session().camera,
				navigation_input,
				interaction_settings,
				delta_seconds);
		}
		LightGizmoProjectionContext light_projection;
		light_projection.light_camera = build_preview_light_camera_context(camera);
		light_projection.view_projection = camera.projection * camera.view;
		light_projection.viewport_size = glm::vec2(preview_rect.GetWidth(), preview_rect.GetHeight());
		light_projection.pointer_position = glm::vec2(
			io.MousePos.x - preview_rect.Min.x,
			io.MousePos.y - preview_rect.Min.y);
		current_preview_session().lighting.hovered_light_id = LightGizmoPickingService().pick(
			current_preview_session().lighting, light_projection);
		const bool light_gizmo_hovered =
			!current_preview_session().lighting.hovered_light_id.empty();
		if (light_gizmo_hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			current_preview_session().lighting.selected_light_id =
				current_preview_session().lighting.hovered_light_id;
			current_preview_session().selection.hovered_instance_index = -1;
			current_preview_session().selection.pointer_selection_armed = false;
		}
		if (interaction_settings.picking_and_highlighting_enabled && !light_gizmo_hovered) {
			current_preview_session().selection.hovered_instance_index =
				pick_preview_instance_index(camera, preview_rect, context);
		} else {
			current_preview_session().selection.hovered_instance_index = -1;
			if (!interaction_settings.picking_and_highlighting_enabled || light_gizmo_hovered) {
				current_preview_session().selection.pointer_selection_armed = false;
			}
		}
		if (interaction_settings.picking_and_highlighting_enabled &&
		    !light_gizmo_hovered &&
		    ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			scene_selection_controller.armPointerSelection(
				current_preview_session().selection,
				io.MousePos.x,
				io.MousePos.y);
		}
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			current_preview_session().camera.orbitByPixels(io.MouseDelta.x, io.MouseDelta.y);
		}
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
			const float viewport_scale =
				std::max(1.0f, std::min(available.x, available.y));
			current_preview_session().camera.panByPixels(
				io.MouseDelta.x,
				io.MouseDelta.y,
				viewport_scale);
		}
		if (io.MouseWheel != 0.0f) {
			current_preview_session().camera.zoomByWheel(
				io.MouseWheel,
				std::max(1.0f, std::min(available.x, available.y)));
		}
		if (interaction_settings.picking_and_highlighting_enabled &&
		    current_preview_session().selection.pointer_selection_armed &&
		    ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			if (scene_selection_controller.completePointerSelection(
				    current_preview_session().selection,
				    io.MousePos.x,
				    io.MousePos.y,
				    5.0f)) {
				select_preview_instance_in_editor(app_state,
				                                 context,
				                                 current_preview_session().selection.selected_instance_index);
			}
		}
	} else {
		if (!preview_hovered) {
			current_preview_session().selection.hovered_instance_index = -1;
			current_preview_session().lighting.hovered_light_id = LightId();
		}
		if (current_preview_session().selection.pointer_selection_armed &&
		    ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
			current_preview_session().selection.pointer_selection_armed = false;
		}
	}
}

namespace {

void draw_smart_editor(EditorWorkspaceSession *app_state)
{
	if (app_state == nullptr) {
		return;
	}

	sync_smart_editor_state(app_state->document.source_text);
	std::vector<std::string> lines = current_editor_lines(app_state->document.source_text);
	GrammarSymbolIndex symbols = GrammarSymbolIndex::fromSourceLines(lines);

	ImGui::PushStyleColor(ImGuiCol_ChildBg, color_from_hex(0xffffff));
	ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xd7e0ea, 0.95f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
	ImGui::BeginChild("SmartEditorRegion", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_NoScrollbar);
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);
	if (app_fonts.mono != nullptr) {
		ImGui::PushFont(app_fonts.mono);
	}

	if (app_state->document.source_text.capacity() < 2048) {
		app_state->document.source_text.reserve(2048);
	}

	const float footer_height = ImGui::GetTextLineHeightWithSpacing() + 6.0f;
	ImVec2 editor_size = ImGui::GetContentRegionAvail();
	editor_size.y = std::max(120.0f, editor_size.y - footer_height);

	ImGui::SetWindowFontScale(app_state->layout.editor_font_scale);

	ImGui::PushStyleColor(ImGuiCol_FrameBg, color_from_hex(0xffffff));
	ImGui::PushStyleColor(ImGuiCol_Border, color_from_hex(0xd7e0ea, 0.95f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);

	if (smart_editor_state.request_focus) {
		ImGui::SetKeyboardFocusHere();
		smart_editor_state.request_focus = false;
	}

	EditorLineInputCallbackUserData callback_data{
		&app_state->document.source_text,
		&smart_editor_state.cursor_column,
		&smart_editor_state.selection_start,
		&smart_editor_state.selection_end};
	const ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_CallbackResize |
	                                        ImGuiInputTextFlags_CallbackAlways |
	                                        ImGuiInputTextFlags_AllowTabInput;
	const bool text_changed = ImGui::InputTextMultiline("##smart_editor_document",
	                                                    app_state->document.source_text.data(),
	                                                    app_state->document.source_text.capacity() + 1,
	                                                    editor_size,
	                                                    input_flags,
	                                                    editor_line_input_callback,
	                                                    &callback_data);
	const ImGuiID editor_id = ImGui::GetItemID();
	const ImRect editor_rect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
	const bool editor_hovered = ImGui::IsItemHovered();
	const bool editor_active = ImGui::IsItemActive();
	ImGuiIO &io = ImGui::GetIO();

	if (editor_hovered && io.KeyCtrl && io.MouseWheel != 0.0f) {
		const float zoom_factor = std::pow(1.10f, io.MouseWheel);
		app_state->layout.editor_font_scale =
			std::clamp(app_state->layout.editor_font_scale * zoom_factor,
			           kMinEditorFontScale,
			           kMaxEditorFontScale);
		ImGui::SetWindowFontScale(app_state->layout.editor_font_scale);
		ImGui::SetItemUsingMouseWheel();
	}

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
		commit_editor_text(app_state, app_state->document.source_text);
		lines = current_editor_lines(app_state->document.source_text);
		symbols = GrammarSymbolIndex::fromSourceLines(lines);
	}
	const bool has_identifier_selection = refresh_selected_identifier_from_selection(lines, symbols);
	const EditorTokenMatch cursor_position = position_for_document_offset(lines, smart_editor_state.cursor_column);
	const std::vector<EditorDiagnostic> diagnostics = build_editor_diagnostics(lines, app_state);
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
	draw_editor_caret_overlay(lines,
	                          symbols,
	                          editor_id,
	                          editor_rect,
	                          smart_editor_state.cursor_column);
	draw_part_class_autocomplete(app_state,
	                             lines,
	                             symbols,
	                             cursor_position,
	                             editor_id,
	                             editor_rect,
	                             editor_active,
	                             editor_hovered);
	draw_material_autocomplete(app_state,
	                           lines,
	                           symbols,
	                           cursor_position,
	                           editor_id,
	                           editor_rect);
	if (editor_hovered && !ImGui::IsPopupOpen("IdentifierSelectionTooltip")) {
		if (!draw_editor_diagnostic_tooltip(lines, symbols, diagnostics, editor_id, editor_rect)) {
			draw_hovered_editor_token_tooltip(lines, symbols, editor_id, editor_rect);
		}
	}
	if (has_identifier_selection && editor_hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
		ImGui::OpenPopup("IdentifierSelectionTooltip");
	}

	ImGui::SetWindowFontScale(1.0f);
	if (app_fonts.mono != nullptr) {
		ImGui::PopFont();
	}

	GrammarEditorStatusPresentation editor_status;
	editor_status.line_number = cursor_position.line + 1;
	editor_status.column_number = cursor_position.column + 1;
	editor_status.line_count = static_cast<int>(lines.size());
	editor_status.zoom_percent =
		static_cast<int>(std::round(app_state->layout.editor_font_scale * 100.0f));
	editor_status.diagnostic_count = static_cast<int>(diagnostics.size());
	editor_status.identifier_match_count = identifier_matches.found
		                                       ? static_cast<int>(identifier_matches.ranges.size())
		                                       : 0;
	editor_status.bracket_near_cursor = bracket_match.found;
	editor_status.bracket_has_pair = bracket_match.has_pair;
	editor_status.bracket_character = bracket_match.delimiter;
	if (smart_editor_state.has_selected_identifier) {
		editor_status.selected_identifier = smart_editor_state.selected_identifier_token.text;
	}
	editor_status.auto_run_scheduled = scene_generation_scheduled();
	editor_status.auto_run_remaining_seconds = static_cast<float>(
		std::max(0.0, scene_generation_scheduled_start_time() - glfwGetTime()));
	grammar_editor_panel.drawStatus(
		editor_status,
		[](const std::string &text,
		   unsigned int fill_color,
		   unsigned int text_color,
		   float fill_alpha,
		   float border_alpha) {
			draw_status_chip(text, fill_color, text_color, fill_alpha, border_alpha);
		});

	GrammarSymbolInspection symbol_inspection;
	GrammarSymbolInspection *symbol_inspection_pointer = nullptr;
	if (smart_editor_state.has_selected_identifier) {
		symbol_inspection.selected_token = smart_editor_state.selected_identifier_token;
		symbol_inspection.selected_line = smart_editor_state.selected_identifier_line;
		symbol_inspection.selected_column = smart_editor_state.selected_identifier_column;
		symbol_inspection.occurrence_ranges =
			collect_token_occurrence_ranges(lines, symbols, symbol_inspection.selected_token);
		const int selected_end_column =
			symbol_inspection.selected_column +
			static_cast<int>(symbol_inspection.selected_token.text.size());
		symbol_inspection.active_occurrence_index =
			find_token_occurrence_index(symbol_inspection.occurrence_ranges,
			                            symbol_inspection.selected_line,
			                            symbol_inspection.selected_column,
			                            selected_end_column);
		symbol_inspection_pointer = &symbol_inspection;
	}
	grammar_symbol_inspector_panel.drawPopup(
		symbol_inspection_pointer,
		draw_selected_identifier_details,
		[&lines](const EditorRange &source_range) {
			select_editor_range(lines,
			                    source_range.line,
			                    source_range.start_column,
			                    source_range.end_column);
		});

	ImGui::EndChild();
}

ScenePrimitiveInspection build_scene_primitive_inspection()
{
	ScenePrimitiveInspection inspection;
	const Context *context = current_scene_snapshot() != nullptr
		                         ? current_scene_snapshot()->generationContext()
		                         : nullptr;
	const int instance_index = current_preview_session().selection.selected_instance_index;
	if (!is_valid_preview_instance_index(const_cast<Context *>(context), instance_index)) {
		return inspection;
	}

	const PrimitiveInstance &instance =
		context->primitive_instances[static_cast<std::size_t>(instance_index)];
	const PrimitiveBounds bounds = context->getInstanceBounds(instance);
	inspection.available = true;
	inspection.instance_index = instance_index;
	inspection.primitive_type = instance.type;
	inspection.material_name = instance.material_name;
	inspection.bounds_min = {bounds.min.x, bounds.min.y, bounds.min.z};
	inspection.bounds_max = {bounds.max.x, bounds.max.y, bounds.max.z};
	inspection.bounds_center = {bounds.center.x, bounds.center.y, bounds.center.z};
	inspection.position = {instance.position.x, instance.position.y, instance.position.z};
	inspection.velocity = {instance.velocity.x, instance.velocity.y, instance.velocity.z};
	inspection.rotational_velocity = {
		instance.rotational_velocity.x,
		instance.rotational_velocity.y,
		instance.rotational_velocity.z};
	inspection.rotation_degrees = {
		instance.rotation_degrees.x,
		instance.rotation_degrees.y,
		instance.rotation_degrees.z};
	inspection.mass = instance.mass;
	inspection.immovable = instance.immovable;
	if (instance.resolved_geometry) {
		inspection.watertight = instance.resolved_geometry->isWatertight();
		switch (instance.resolved_geometry->volumeEvidence().kind()) {
		case PrimitiveVolumeEvidenceKind::Analytic:
			inspection.volume_evidence = "Analytic";
			break;
		case PrimitiveVolumeEvidenceKind::MeshDerived:
			inspection.volume_evidence = "Mesh-derived";
			break;
		case PrimitiveVolumeEvidenceKind::Undefined:
			inspection.volume_evidence = "Undefined";
			break;
		}
		switch (instance.resolved_geometry->collisionPolicy()) {
		case PrimitiveCollisionPolicy::Disabled:
			inspection.collision_policy = "Disabled";
			break;
		case PrimitiveCollisionPolicy::StaticTriangleMesh:
			inspection.collision_policy = "Static triangle mesh";
			break;
		case PrimitiveCollisionPolicy::ConvexMesh:
			inspection.collision_policy = "Convex mesh";
			break;
		}
		std::ostringstream topology_hash;
		topology_hash << std::hex << instance.resolved_geometry->topologyHash();
		inspection.topology_hash = topology_hash.str();
		if (instance.resolved_geometry->hasTriangleMesh()) {
			inspection.generated_vertex_count =
				instance.resolved_geometry->triangleMesh().vertices.size();
			inspection.generated_triangle_count =
				instance.resolved_geometry->triangleMesh().faces.size();
		}
		if (instance.resolved_geometry->volumeEvidence().hasVolume()) {
			const float volume_scale = std::fabs(
				glm::determinant(glm::mat3(instance.primary_transform)));
			const float world_volume =
				instance.resolved_geometry->volumeEvidence().localVolume() * volume_scale;
			if (world_volume > 0.000001f) {
				inspection.estimated_density = instance.mass / world_volume;
			}
		}
	}
	if (instance.shape_specification) {
		inspection.shape_specification = instance.shape_specification->canonicalText();
		const auto *axial_profile =
			dynamic_cast<const AxialProfileShapeSpecification *>(
				instance.shape_specification.get());
		if (axial_profile != nullptr) {
			inspection.profile_count = axial_profile->profiles().size();
			inspection.level_count = axial_profile->levels().size();
			inspection.closed_profile_geometry =
				axial_profile->capPolicy().closesBothEnds();
			for (const AxialProfileLevel &level : axial_profile->levels()) {
				switch (level.transition()) {
				case AxialTransitionKind::Initial:
					break;
				case AxialTransitionKind::Hold:
					++inspection.hold_transition_count;
					break;
				case AxialTransitionKind::Linear:
					++inspection.linear_transition_count;
					break;
				case AxialTransitionKind::Step:
					++inspection.step_transition_count;
					break;
				}
			}
		}
		const auto *tapered_sweep =
			dynamic_cast<const TaperedSweepShapeSpecification *>(
				instance.shape_specification.get());
		if (tapered_sweep != nullptr) {
			inspection.vegetation_geometry = true;
			inspection.vegetation_geometry_kind = "Tapered branch or stem";
			inspection.vegetation_path_sample_count =
				tapered_sweep->pathPoints().size();
			if (!tapered_sweep->radii().empty()) {
				inspection.vegetation_base_radius = tapered_sweep->radii().front();
				inspection.vegetation_tip_radius = tapered_sweep->radii().back();
			}
			inspection.vegetation_radial_segments =
				tapered_sweep->circumferentialSegments();
			inspection.vegetation_cap_policy =
				tapered_sweep->capPolicy().canonicalText();
		}
		const auto *botanical_blade =
			dynamic_cast<const BotanicalBladeShapeSpecification *>(
				instance.shape_specification.get());
		if (botanical_blade != nullptr) {
			inspection.vegetation_geometry = true;
			inspection.vegetation_geometry_kind =
				botanical_blade->family() == ShapeFamily::PetalBlade
					? "Petal blade"
					: "Leaf blade";
			inspection.botanical_blade_profile =
				botanicalBladeProfileName(botanical_blade->profile());
			inspection.botanical_blade_length = botanical_blade->length();
			inspection.botanical_blade_width = botanical_blade->maximumWidth();
			inspection.botanical_blade_curvature =
				botanical_blade->longitudinalCurvature();
			inspection.botanical_blade_camber = botanical_blade->camber();
			inspection.botanical_blade_twist_degrees =
				botanical_blade->twistDegrees();
			inspection.botanical_blade_thickness = botanical_blade->thickness();
			inspection.botanical_blade_width_power = botanical_blade->widthPower();
			inspection.botanical_blade_longitudinal_segments =
				botanical_blade->longitudinalSegments();
				inspection.botanical_blade_lateral_segments =
					botanical_blade->lateralSegments();
			}
			const auto *plant = dynamic_cast<const PlantShapeSpecification *>(
				instance.shape_specification.get());
			if (plant != nullptr) {
				inspection.vegetation_geometry = true;
				inspection.vegetation_geometry_kind = "Species-driven plant assembly";
				inspection.plant_species = plant->species().identifier();
				inspection.plant_architecture =
					plantArchitectureName(plant->species().architecture());
				inspection.plant_development_state =
					plantDevelopmentStateName(plant->developmentState());
				inspection.plant_age = plant->age();
				inspection.plant_seed = plant->deterministicSeed();
				inspection.plant_detail_level =
					geometryDetailLevelName(plant->detailLevel());
			}
			const auto *vine = dynamic_cast<const VineShapeSpecification *>(
				instance.shape_specification.get());
			if (vine != nullptr) {
				inspection.vegetation_geometry = true;
				inspection.vegetation_geometry_kind =
					"Guided vine path and organ assembly";
				inspection.plant_species = vine->species().identifier();
				inspection.plant_architecture = "Vine";
				inspection.plant_detail_level =
					geometryDetailLevelName(vine->detailLevel());
				inspection.vine_growth_mode =
					vineGrowthModeName(vine->growth().growthMode());
				inspection.vine_collision_behavior =
					vegetationCollisionBehaviorName(
						vine->growth().collisionBehavior());
				inspection.vine_attachment_mode =
					surfaceAttachmentModeName(vine->growth().attachmentMode());
				inspection.vine_target_identifier = vine->target().has_value()
					? vine->target()->identifier() : "<none>";
				inspection.vine_obstacle_count = vine->obstacles().size();
				inspection.vine_segment_count = vine->growth().maximumSegments();
				inspection.vine_step_length = vine->growth().stepLength();
				inspection.vine_attachment_distance =
					vine->growth().attachmentDistance();
			}
			const auto *scatter =
				dynamic_cast<const ScatterRegionShapeSpecification *>(
					instance.shape_specification.get());
			if (scatter != nullptr) {
				inspection.vegetation_geometry = true;
				inspection.vegetation_geometry_kind =
					"Deterministic vegetation scatter assembly";
				inspection.plant_species = scatter->species().identifier();
				inspection.plant_architecture =
					plantArchitectureName(scatter->species().architecture());
				inspection.plant_detail_level =
					geometryDetailLevelName(scatter->detailLevel());
				inspection.scatter_region_identifier =
					scatter->scatter().identifier();
				inspection.scatter_surface_identifier =
					scatter->scatter().surface().identifier();
				inspection.scatter_surface_face =
					scatterSurfaceFaceName(scatter->scatter().surfaceFace());
				inspection.scatter_orientation_mode =
					scatterOrientationModeName(
						scatter->scatter().orientationMode());
				inspection.scatter_placement_layer = collisionLayerName(
					scatter->scatter().collisionPolicy().placementLayer());
				inspection.scatter_collision_mask_bits =
					scatter->scatter().collisionPolicy().collisionMask().bits();
				inspection.scatter_seed = scatter->deterministicSeed();
				inspection.scatter_density = scatter->scatter().density();
				inspection.scatter_minimum_distance =
					scatter->scatter().minimumDistance();
				inspection.scatter_scale_range = {
					scatter->scatter().scaleRange().x,
					scatter->scatter().scaleRange().y};
				inspection.scatter_obstacle_count = scatter->obstacles().size();
			}
	}
	const std::optional<SourceRange> source_range =
		current_preview_session().source_associations.sourceRangeForInstance(instance_index);
	if (source_range.has_value()) {
		inspection.source_range = *source_range;
	}
	inspection.connection_evidence =
		"The current scene context exposes connection overlays but does not retain a per-primitive authored connection record.";
	return inspection;
}

std::vector<SpatialObjectSelectionEntry> build_spatial_object_selection_entries()
{
	const GeneratedSceneSnapshot *snapshot = current_scene_snapshot().get();
	const SpatialBuildingModel *model =
		snapshot != nullptr ? snapshot->spatialBuildingModel() : nullptr;
	return model != nullptr
		? spatial_object_inspection_service.buildContainmentSelection(*model)
		: std::vector<SpatialObjectSelectionEntry>();
}

SpatialObjectInspection build_spatial_object_inspection()
{
	const GeneratedSceneSnapshot *snapshot = current_scene_snapshot().get();
	const SpatialBuildingModel *model =
		snapshot != nullptr ? snapshot->spatialBuildingModel() : nullptr;
	const SmallModernBuildingModel *building_model =
		snapshot != nullptr ? snapshot->smallModernBuildingModel() : nullptr;
	if (model == nullptr) return {};

	EditorSelection &selection = current_preview_session().selection;
	if (selection.selected_instance_index >= 0) {
		const SpatialObjectInspection primitive_object =
			spatial_object_inspection_service.inspectPrimitive(
				*model,
				building_model,
				static_cast<std::size_t>(selection.selected_instance_index));
		if (primitive_object.available) {
			selection.selected_spatial_object_id = primitive_object.object_id;
			return primitive_object;
		}
	}

	if (selection.selected_spatial_object_id.empty()) {
		selection.selected_spatial_object_id =
			model->containmentTree().rootObjectId().value();
	}
	return spatial_object_inspection_service.inspectObject(
		*model,
		building_model,
		SpatialObjectId(selection.selected_spatial_object_id));
}

void draw_editor(EditorWorkspaceSession *app_state)
{
	const std::string document_label = display_document_name(app_state);
	draw_section_heading("Grammar");
	ImGui::SameLine();
	draw_status_chip(document_label, 0x6f95ba, 0x436786);
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
		if (!app_state->document.local_path.empty()) {
			ImGui::SetTooltip("%s", app_state->document.local_path.string().c_str());
		} else {
			ImGui::SetTooltip("This document has not been saved locally.");
		}
	}
	ImGui::SameLine();
	draw_status_chip("Webhost Backend", 0x93a9bc, 0x5a7187);
	if (app_state != nullptr && !app_state->storage_identity.cloud_file_id.empty()) {
		ImGui::SameLine();
		draw_status_chip("Cloud linked", 0x7ea88e, 0x527564, 0.12f, 0.24f);
	}
	if (app_state != nullptr && app_state->document.dirty) {
		ImGui::SameLine();
		draw_status_chip("Modified", 0xc8a766, 0x7a6035, 0.14f, 0.26f);
	}
	if (app_state != nullptr && app_state->storage_identity.cloud_published) {
		ImGui::SameLine();
		draw_status_chip("Published", 0x7ea88e, 0x527564, 0.12f, 0.24f);
	}
	if (scene_generation_in_progress()) {
		ImGui::SameLine();
		draw_status_chip(scene_generation_queued() ? "Queued update" : "Regenerating",
		                 0xc8a766,
		                 0x7a6035,
		                 0.14f,
		                 0.26f);
	} else if (app_state != nullptr && scene_generation_scheduled()) {
		const float remaining_seconds =
			std::max(0.0, scene_generation_scheduled_start_time() - glfwGetTime());
		char autorun_label[48];
		std::snprintf(autorun_label, sizeof(autorun_label), "Auto-run %.1fs", remaining_seconds);
		ImGui::SameLine();
		draw_status_chip(autorun_label, 0xc8a766, 0x7a6035, 0.14f, 0.26f);
	}

	const ImGuiIO &interface_io = ImGui::GetIO();
	const bool open_shortcut = interface_io.KeyCtrl && !interface_io.KeyShift &&
		ImGui::IsKeyPressed(ImGuiKey_O, false);
	const bool save_shortcut = interface_io.KeyCtrl && !interface_io.KeyShift &&
		ImGui::IsKeyPressed(ImGuiKey_S, false);
	const bool save_as_shortcut = interface_io.KeyCtrl && interface_io.KeyShift &&
		ImGui::IsKeyPressed(ImGuiKey_S, false);

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));
	if (ImGui::Button("New")) {
		request_document_replacement(app_state,
		                             DocumentReplacementAction::CreateNewDocument);
	}
	ImGui::SameLine();
	if (ImGui::Button("Open...") || open_shortcut) {
		request_document_replacement(app_state,
		                             DocumentReplacementAction::OpenLocalDocument);
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
		ImGui::SetTooltip("Open a local .p3d or .grammar file (Ctrl+O)");
	}
	ImGui::SameLine();
	if (ImGui::Button("Save") || save_shortcut) {
		save_current_document(app_state, false);
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
		ImGui::SetTooltip("Save the current document (Ctrl+S)");
	}
	ImGui::SameLine();
	if (ImGui::Button("Save As...") || save_as_shortcut) {
		request_local_file_dialog(app_state, LocalFileDialogPurpose::SaveGrammarAs);
	}
	if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) {
		ImGui::SetTooltip("Save to a new local file (Ctrl+Shift+S)");
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(!app_state->authentication.session.authenticated);
	if (ImGui::Button("Cloud Open")) {
		request_document_replacement(app_state,
		                             DocumentReplacementAction::OpenCloudDocument);
	}
	ImGui::SameLine();
	if (ImGui::Button("Cloud Save")) {
		if (app_state->storage_identity.cloud_file_id.empty()) {
			open_cloud_dialog(app_state, CloudDialogAction::SaveAs);
		} else {
			save_document_to_cloud(app_state, default_cloud_title(app_state), false);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Cloud Save As")) {
		open_cloud_dialog(app_state, CloudDialogAction::SaveAs);
	}
	ImGui::SameLine();
	if (ImGui::Button("Publish")) {
		publish_document_to_cloud(app_state);
	}
	ImGui::SameLine();
	if (ImGui::Button("Depublish")) {
		depublish_document_from_cloud(app_state);
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	push_primary_action_button_style();
	if (ImGui::Button("Run")) {
		clear_auto_run_timer(app_state);
		regenerate_scene(app_state->document.source_text);
	}
	pop_primary_action_button_style();
	ImGui::SameLine();
	if (ImGui::Button("Export PLY")) {
		std::filesystem::path export_path(default_cloud_title(app_state));
		if (export_path.empty()) {
			export_path = "untitled";
		}
		export_path.replace_extension(".ply");
		export_ply(export_path.string());
	}
	ImGui::PopStyleVar();
	if (!app_state->local_document_workflow.status_message.empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text,
		                      app_state->local_document_workflow.status_is_error
		                          ? color_from_hex(0xb85d5d)
		                          : color_from_hex(0x527564));
		ImGui::TextWrapped("%s", app_state->local_document_workflow.status_message.c_str());
		ImGui::PopStyleColor();
	}

	if (ImGui::BeginTabBar("WorkspaceTabs", ImGuiTabBarFlags_FittingPolicyResizeDown)) {
		if (ImGui::BeginTabItem("Editor")) {
			draw_smart_editor(app_state);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Diagnostics")) {
			const std::vector<std::string> source_lines = current_editor_lines(app_state->document.source_text);
			const std::vector<EditorDiagnostic> diagnostics =
				build_editor_diagnostics(source_lines, app_state);
			diagnostics_panel.draw(
				diagnostics,
				[&source_lines](const EditorRange &source_range) {
					if (!source_range.found) {
						return;
					}
					select_editor_range(source_lines,
					                    source_range.line,
					                    source_range.start_column,
					                    source_range.end_column);
				});
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Inspector")) {
			scene_inspector_panel.draw(
				build_scene_primitive_inspection(),
				current_preview_session().statistics);
			spatial_object_inspector_panel.draw(
				build_spatial_object_inspection(),
				build_spatial_object_selection_entries(),
				&current_preview_session().selection.selected_spatial_object_id,
				&current_preview_session().selection.selected_instance_index);
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("AI")) {
			ImGui::BeginChild("AiTabBody", ImVec2(0.0f, 0.0f), false);
			ai_assistant_panel.draw(*app_state);
			ImGui::EndChild();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Materials")) {
			ImGui::BeginChild("MaterialsTabBody", ImVec2(0.0f, 0.0f), false);
			material_library_panel.draw(*app_state);
			ImGui::EndChild();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Lights")) {
			ImGui::BeginChild("LightingTabBody", ImVec2(0.0f, 0.0f), false);
			lighting_panel.draw(&current_preview_session().lighting);
			ImGui::EndChild();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Electrical Controls")) {
			ImGui::BeginChild("ElectricalControlsTabBody", ImVec2(0.0f, 0.0f), false);
			electrical_controls_panel.draw(&current_preview_session().lighting);
			ImGui::EndChild();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Render Settings")) {
			ImGui::BeginChild("RenderSettingsTabBody", ImVec2(0.0f, 0.0f), false);
			render_settings_panel.draw(*app_state);
			ImGui::EndChild();
			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	cloud_document_dialog.draw(*app_state);
	draw_unsaved_changes_dialog(app_state);
}

void draw_authentication_panel_content(EditorWorkspaceSession &workspace_session)
{
	EditorWorkspaceSession *app_state = &workspace_session;
	if (app_state == nullptr) {
		return;
	}

	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	if (viewport == nullptr) {
		return;
	}

	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
	                                ImGuiWindowFlags_NoMove |
	                                ImGuiWindowFlags_NoSavedSettings;
	ImGui::Begin("Progen3dLogin", nullptr, window_flags);

	ImDrawList *draw_list = ImGui::GetWindowDrawList();
	const ImVec2 background_min = viewport->Pos;
	const ImVec2 background_max(viewport->Pos.x + viewport->Size.x, viewport->Pos.y + viewport->Size.y);
	draw_list->AddRectFilledMultiColor(background_min,
	                                   background_max,
	                                   ImGui::GetColorU32(color_from_hex(0xf6f8fc)),
	                                   ImGui::GetColorU32(color_from_hex(0xebf3fb)),
	                                   ImGui::GetColorU32(color_from_hex(0xeaf0f8)),
	                                   ImGui::GetColorU32(color_from_hex(0xf6f8fc)));
	draw_list->AddCircleFilled(ImVec2(background_max.x - 120.0f, background_min.y + 140.0f),
	                           220.0f,
	                           ImGui::GetColorU32(color_from_hex(0xb7d0ea, 0.12f)),
	                           72);
	draw_list->AddCircleFilled(ImVec2(background_min.x + 160.0f, background_max.y - 120.0f),
	                           200.0f,
	                           ImGui::GetColorU32(color_from_hex(0x97b8d7, 0.10f)),
	                           72);

	const float card_width = std::clamp(viewport->Size.x * 0.72f, 760.0f, 980.0f);
	const float card_height = std::clamp(viewport->Size.y * 0.62f, 460.0f, 580.0f);
	ImGui::SetCursorPos(ImVec2((viewport->Size.x - card_width) * 0.5f,
	                           (viewport->Size.y - card_height) * 0.5f));

	begin_surface("FirebaseLoginCard", ImVec2(card_width, card_height), true, ImVec2(18.0f, 18.0f));
	FirebaseAuthenticationService authentication_service;

	auto persist_firebase_config = [&]() {
		FirebaseAuthConfig config = current_auth_config_from_inputs(app_state);
		if (!config.valid()) {
			set_auth_feedback(app_state, "Firebase API key and project ID are required.", true);
			return false;
		}
		std::string error;
		if (!save_firebase_auth_config(config, &error)) {
			set_auth_feedback(app_state, error, true);
			return false;
		}
		config.loaded_from = firebase_auth_default_config_path();
		load_firebase_config_into_ui(app_state, config);
		set_auth_feedback(app_state, "Saved Firebase config to " + config.loaded_from + ".", false);
		return true;
	};

	auto authenticate_with_email = [&]() {
		FirebaseAuthConfig config = current_auth_config_from_inputs(app_state);
		const std::string email = text_buffer_value(app_state->authentication.email);
		const std::string password = text_buffer_value(app_state->authentication.password);
		if (!config.valid()) {
			set_auth_feedback(app_state, "Firebase API key and project ID are required.", true);
			return;
		}
		if (email.empty() || password.empty()) {
			set_auth_feedback(app_state, "Enter your email and password to continue.", true);
			return;
		}

		app_state->authentication.config = config;
		FirebaseAuthSession session;
		std::string error;
		if (!authentication_service.signInWithPassword(config,
		                                              email,
		                                              password,
		                                              &session,
		                                              &error)) {
			set_auth_feedback(app_state, error, true);
			return;
		}
		std::string backend_error;
		const bool backend_synced = synchronize_backend_login(app_state, &session, &backend_error);

		if (app_state->authentication.config.loaded_from.empty()) {
			std::string save_config_error;
			if (save_firebase_auth_config(config, &save_config_error)) {
				app_state->authentication.config.loaded_from = firebase_auth_default_config_path();
			} else {
				debugout("Firebase login succeeded but saving config failed: " + save_config_error);
			}
		}

		std::string save_session_error;
		if (!save_firebase_auth_session(session, &save_session_error)) {
			debugout("Firebase login succeeded but saving session failed: " + save_session_error);
		}
		apply_authenticated_session(app_state,
		                            session,
		                            session.email.empty()
		                                ? "Authenticated with Firebase."
		                                : "Authenticated with Firebase as " + session.email + ".");
		if (!backend_synced) {
			const std::string message_prefix =
				session.email.empty()
					? "Authenticated with Firebase."
					: "Authenticated with Firebase as " + session.email + ".";
			set_auth_feedback(app_state,
			                  message_prefix + " Cloud backend sync is unavailable: " + backend_error,
			                  true);
		}
	};

	auto authenticate_with_google = [&]() {
		FirebaseAuthConfig config = current_auth_config_from_inputs(app_state);
		if (!config.valid()) {
			set_auth_feedback(app_state, "Firebase API key and project ID are required.", true);
			return;
		}

		app_state->authentication.config = config;
		FirebaseAuthSession session;
		std::string error;
		if (!authentication_service.signInWithGoogle(config, &session, &error)) {
			set_auth_feedback(app_state, error, true);
			return;
		}
		std::string backend_error;
		const bool backend_synced = synchronize_backend_login(app_state, &session, &backend_error);

		if (app_state->authentication.config.loaded_from.empty()) {
			std::string save_config_error;
			if (save_firebase_auth_config(config, &save_config_error)) {
				app_state->authentication.config.loaded_from = firebase_auth_default_config_path();
			} else {
				debugout("Google sign-in succeeded but saving config failed: " + save_config_error);
			}
		}

		std::string save_session_error;
		if (!save_firebase_auth_session(session, &save_session_error)) {
			debugout("Google sign-in succeeded but saving session failed: " + save_session_error);
		}
		apply_authenticated_session(app_state,
		                            session,
		                            session.email.empty()
		                                ? "Authenticated with Google."
		                                : "Authenticated with Google as " + session.email + ".");
		if (!backend_synced) {
			const std::string message_prefix =
				session.email.empty()
					? "Authenticated with Google."
					: "Authenticated with Google as " + session.email + ".";
			set_auth_feedback(app_state,
			                  message_prefix + " Cloud backend sync is unavailable: " + backend_error,
			                  true);
		}
	};

	ImGui::BeginChild("LoginCardBody", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_NoScrollbar);
	if (app_fonts.title != nullptr) {
		ImGui::PushFont(app_fonts.title);
	}
	ImGui::TextUnformatted("Sign In");
	if (app_fonts.title != nullptr) {
		ImGui::PopFont();
	}
	ImGui::Dummy(ImVec2(0.0f, 10.0f));

	if (app_state->authentication.config.valid()) {
		draw_status_chip(app_state->authentication.config.project_id, 0x7ea88e, 0x527564, 0.12f, 0.24f);
	}

	draw_divider();

	if (!app_state->authentication.status_message.empty()) {
		ImGui::PushStyleColor(ImGuiCol_Text,
		                      app_state->authentication.status_is_error
		                          ? color_from_hex(0xb85d5d)
		                          : color_from_hex(0x527564));
		ImGui::TextWrapped("%s", app_state->authentication.status_message.c_str());
		ImGui::PopStyleColor();
		ImGui::Dummy(ImVec2(0.0f, 6.0f));
	}

	push_primary_action_button_style();
	if (ImGui::Button("Continue with Google", ImVec2(-1.0f, 38.0f))) {
		authenticate_with_google();
		if (app_state->authentication.session.authenticated) {
			authentication_panel_is_visible() = false;
		}
	}
	pop_primary_action_button_style();
	if (ImGui::Button("Continue Offline", ImVec2(-1.0f, 34.0f))) {
		authentication_panel_is_visible() = false;
		set_auth_feedback(app_state, "Using the local editor without cloud services.", false);
	}

	ImGui::Dummy(ImVec2(0.0f, 8.0f));
	draw_divider();
	ImGui::Dummy(ImVec2(0.0f, 6.0f));

	bool sign_in_requested = false;
	ImGui::PushItemWidth(-1.0f);
	ImGui::InputTextWithHint("##FirebaseEmail",
	                         "name@example.com",
	                         app_state->authentication.email.data(),
	                         app_state->authentication.email.size());
	ImGuiInputTextFlags password_flags = ImGuiInputTextFlags_EnterReturnsTrue;
	if (!app_state->authentication.show_password) {
		password_flags |= ImGuiInputTextFlags_Password;
	}
	sign_in_requested =
		ImGui::InputTextWithHint("##FirebasePassword",
		                         "Password",
		                         app_state->authentication.password.data(),
		                         app_state->authentication.password.size(),
		                         password_flags);
	ImGui::PopItemWidth();
	ImGui::Checkbox("Show password", &app_state->authentication.show_password);
	ImGui::Dummy(ImVec2(0.0f, 8.0f));

	push_primary_action_button_style();
	if (ImGui::Button("Sign In", ImVec2(150.0f, 34.0f))) {
		sign_in_requested = true;
	}
	pop_primary_action_button_style();
	ImGui::SameLine();
	if (ImGui::Button("Register", ImVec2(150.0f, 34.0f))) {
		std::string browser_error;
		if (!open_url_in_browser("https://www.xoiam.com/register.php", &browser_error)) {
			set_auth_feedback(app_state, browser_error, true);
		}
	}
	if (sign_in_requested) {
		authenticate_with_email();
		if (app_state->authentication.session.authenticated) {
			authentication_panel_is_visible() = false;
		}
	}

	ImGui::Dummy(ImVec2(0.0f, 12.0f));
	if (ImGui::CollapsingHeader("Config")) {
		ImGui::PushItemWidth(-1.0f);
		ImGui::InputTextWithHint("##FirebaseApiKey",
		                         "Firebase API Key",
		                         app_state->authentication.api_key.data(),
		                         app_state->authentication.api_key.size());
		ImGui::InputTextWithHint("##FirebaseProjectId",
		                         "Project ID",
		                         app_state->authentication.project_id.data(),
		                         app_state->authentication.project_id.size());
		ImGui::InputTextWithHint("##GoogleClientId",
		                         "Google OAuth Client ID",
		                         app_state->authentication.google_client_id.data(),
		                         app_state->authentication.google_client_id.size());
		ImGui::InputTextWithHint("##GoogleClientSecret",
		                         "Google OAuth Client Secret (optional)",
		                         app_state->authentication.google_client_secret.data(),
		                         app_state->authentication.google_client_secret.size());
		ImGui::InputTextWithHint("##BackendBaseUrl",
		                         "Backend Base URL",
		                         app_state->authentication.backend_base_url.data(),
		                         app_state->authentication.backend_base_url.size());
		ImGui::PopItemWidth();
		ImGui::Dummy(ImVec2(0.0f, 6.0f));
		if (ImGui::Button("Reload Config", ImVec2(120.0f, 0.0f))) {
			load_detected_firebase_config(app_state, true);
		}
		ImGui::SameLine();
		if (ImGui::Button("Save Config", ImVec2(120.0f, 0.0f))) {
			persist_firebase_config();
		}
	}

	ImGui::EndChild();

	end_surface();
	ImGui::End();
}

bool draw_preview_timeline_content(EditorWorkspaceSession &workspace_session,
                                   Context *scene_context,
                                   bool grammar_time_driven,
                                   float current_time)
{
	EditorWorkspaceSession *app_state = &workspace_session;
	Context *context = scene_context;
	const float current_simulation_time = current_time;
	bool request_fit_camera = false;

	std::string preview_status = "Ready";
	std::string preview_material_status;
	unsigned int preview_status_fill = 0x6f95ba;
	unsigned int preview_status_text = 0x436786;
	if (is_valid_preview_instance_index(
		    context,
		    current_preview_session().selection.selected_instance_index)) {
		const PrimitiveInstance &instance =
			context->primitive_instances[static_cast<std::size_t>(
				current_preview_session().selection.selected_instance_index)];
		preview_status = "Selected " + instance.type;
		preview_status_fill = 0x4e82b5;
		preview_status_text = 0x355f86;
		if (!instance.material_name.empty()) {
			preview_material_status = instance.material_name;
		}
	} else if (is_valid_preview_instance_index(
		           context,
		           current_preview_session().selection.hovered_instance_index)) {
		const PrimitiveInstance &instance =
			context->primitive_instances[static_cast<std::size_t>(
				current_preview_session().selection.hovered_instance_index)];
		preview_status = "Hover " + instance.type;
		preview_status_fill = 0x8aa4be;
		preview_status_text = 0x576f87;
		if (!instance.material_name.empty()) {
			preview_material_status = instance.material_name;
		}
	}
	char time_buffer[40];
	std::snprintf(time_buffer,
	              sizeof(time_buffer),
	              grammar_time_driven ? "grammar t %.2fs" : "sim t %.2fs",
	              current_simulation_time);
	char camera_buffer[48];
	std::snprintf(camera_buffer,
	              sizeof(camera_buffer),
	              "Camera %.0f deg / %.2f",
	              workspace_session.preview.projection.verticalFieldOfViewDegrees(),
	              preview_camera_distance_value());

	draw_section_heading("Preview");
	ImGui::SameLine();
	draw_status_chip(preview_status, preview_status_fill, preview_status_text);
	if (!preview_material_status.empty()) {
		ImGui::SameLine();
		draw_status_chip(preview_material_status, 0x7ea88e, 0x527564);
	}
	if (scene_generation_in_progress()) {
		ImGui::SameLine();
		draw_status_chip(scene_generation_queued() ? "Queued update" : "Regenerating",
		                 0xc8a766,
		                 0x7a6035,
		                 0.14f,
		                 0.26f);
	}

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));
	if (ImGui::Button(preview_playback_active() ? "Pause" : "Play")) {
		preview_playback_active() = !preview_playback_active();
	}
	ImGui::SameLine();
	if (ImGui::Button("Step")) {
		if (grammar_time_driven) {
			preview_grammar_target_time() = std::max(
				preview_grammar_target_time(),
				getGrammarEvaluationTime(grammar));
			const double sample_time =
				preview_timeline_controller.stepGrammarTime(current_preview_session().timeline);
			request_scene_time_sample_internal(app_state->document.source_text,
			                                   sample_time);
		} else {
			preview_timeline_controller.pause(current_preview_session().timeline);
			step_preview_simulation_once(
				preview_timeline_controller.stepPhysicsTime(current_preview_session().timeline));
		}
	}
	ImGui::SameLine();
	if (ImGui::Button(grammar_time_driven ? "Reset Time" : "Reset Sim")) {
		preview_timeline_controller.reset(current_preview_session().timeline);
		if (grammar_time_driven) {
			request_scene_time_sample_internal(app_state->document.source_text, 0.0);
		} else if (context != nullptr) {
			context->resetSimulation();
			upload_context_preview_buffers(app_state, context);
		}
	}
	ImGui::SameLine();
	if (ImGui::Button("Reset View")) {
		current_preview_session().camera.reset();
		current_preview_session().projection.reset();
		current_preview_session().fit_camera_pending = false;
	}
	ImGui::SameLine();
	if (ImGui::Button("Fit")) {
		request_fit_camera = true;
	}
	ImGui::SameLine();
	push_primary_action_button_style();
	if (ImGui::Button("Run")) {
		clear_auto_run_timer(app_state);
		regenerate_scene(app_state->document.source_text);
	}
	pop_primary_action_button_style();
	ImGui::SameLine();
	if (ImGui::Button(app_state->layout.preview_fullscreen
	                      ? "Windowed"
	                      : "Full Screen")) {
		app_state->layout.preview_fullscreen = !app_state->layout.preview_fullscreen;
	}

	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x667788));
	ImGui::TextUnformatted("Speed");
	ImGui::PopStyleColor();
	ImGui::SameLine();
	ImGui::SetNextItemWidth(110.0f);
	ImGui::SliderFloat("##PreviewSpeed",
	                   &preview_time_scale(),
	                   kMinSimulationTimeScale,
	                   kMaxSimulationTimeScale,
	                   "%.2fx");
	ImGui::SameLine();
	if (grammar_time_driven) {
		float editable_time = static_cast<float>(preview_grammar_target_time());
		ImGui::SetNextItemWidth(124.0f);
		if (ImGui::DragFloat("##GrammarTimeScrub",
		                     &editable_time,
		                     0.01f,
		                     0.0f,
		                     0.0f,
		                     "t %.3fs")) {
			preview_playback_active() = false;
			preview_grammar_target_time() = static_cast<double>(editable_time);
			preview_grammar_request_accumulator() = 0.0;
			request_scene_time_sample_internal(app_state->document.source_text,
			                                   preview_grammar_target_time());
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("Scrub the immutable grammar time input t in seconds. Negative time is supported.");
		}
	} else {
		draw_status_chip(time_buffer, 0x93a9bc, 0x5a7187);
	}
	ImGui::SameLine();
	if (ImGui::Button(camera_buffer)) {
		ImGui::OpenPopup("PreviewCameraOptions");
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Adjust the lens field of view. Mouse-wheel zoom changes camera distance.");
	}
	ImGui::SameLine();
	ImGui::Checkbox("Overlay", &connection_overlay_visible());
	ImGui::SameLine();
	if (ImGui::Checkbox(
			"Picking",
			&workspace_session.preview.interaction_settings.picking_and_highlighting_enabled)) {
		workspace_session.preview.selection.hovered_instance_index = -1;
		workspace_session.preview.selection.pointer_selection_armed = false;
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Enable canvas picking and hovered/selected outlines.");
	}
	ImGui::SameLine();
	if (ImGui::Button("Overlay Options")) {
		ImGui::OpenPopup("PreviewOverlayOptions");
	}
	ImGui::PopStyleVar();

	ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, 0.0f), ImVec2(420.0f, FLT_MAX));
	if (ImGui::BeginPopup("PreviewCameraOptions")) {
		PreviewProjectionConfiguration &projection = workspace_session.preview.projection;
		draw_section_heading("Camera Lens");
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
		ImGui::TextWrapped(
			"Narrow fields of view flatten perspective. Wide fields of view reveal more of the scene and exaggerate depth.");
		ImGui::PopStyleColor();
		draw_divider();

		float editable_field_of_view = projection.verticalFieldOfViewDegrees();
		ImGui::SetNextItemWidth(-1.0f);
		if (ImGui::SliderFloat(
				"Vertical FOV",
				&editable_field_of_view,
				PreviewProjectionConfiguration::minimum_vertical_field_of_view_degrees,
				PreviewProjectionConfiguration::maximum_vertical_field_of_view_degrees,
				"%.0f deg",
				ImGuiSliderFlags_AlwaysClamp)) {
			projection.setVerticalFieldOfViewDegrees(editable_field_of_view);
		}
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip(
				"Lens-only adjustment. Use the mouse wheel over the preview to move the camera closer or farther away.");
		}

		if (ImGui::Button("24 deg Telephoto")) {
			projection.setVerticalFieldOfViewDegrees(24.0f);
		}
		ImGui::SameLine();
		if (ImGui::Button("43 deg Standard")) {
			projection.setVerticalFieldOfViewDegrees(
				PreviewProjectionConfiguration::default_vertical_field_of_view_degrees);
		}
		ImGui::SameLine();
		if (ImGui::Button("70 deg Wide")) {
			projection.setVerticalFieldOfViewDegrees(70.0f);
		}

		draw_divider();
		if (ImGui::Button("Reset Lens")) {
			projection.reset();
		}
		ImGui::SameLine();
		ImGui::Text("Camera distance %.2f", preview_camera_distance_value());
		draw_divider();
		draw_section_heading("Keyboard Navigation");
		ImGui::TextWrapped("Hover the preview and use W/S forward/back, A/D left/right, Q/E down/up, and Z/C to rotate the view left/right around the camera position.");
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::SliderFloat("Movement velocity",
		                   &workspace_session.preview.interaction_settings.navigation_velocity,
		                   0.25f,
		                   20.0f,
		                   "%.2f units/s",
		                   ImGuiSliderFlags_AlwaysClamp);
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::SliderFloat("Rotation velocity",
		                   &workspace_session.preview.interaction_settings.rotation_velocity_degrees,
		                   5.0f,
		                   180.0f,
		                   "%.0f deg/s",
		                   ImGuiSliderFlags_AlwaysClamp);
		ImGui::Text("Current heading %.1f deg", preview_azimuth_degrees());
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopup("PreviewOverlayOptions")) {
		static const char *kPreviewMappingLabels[] = {"Material", "UV", "Triplanar"};
		static const char *kPreviewDebugLabels[] = {"Shaded", "UVs", "Checker", "Blend Weights"};
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
		ImGui::TextUnformatted("Preview overlays");
		ImGui::PopStyleColor();
		draw_divider();
		ImGui::Checkbox("Connection overlay", &connection_overlay_visible());
		if (connection_overlay_visible()) {
			ImGui::Checkbox("Axes", &connection_axes_visible());
			ImGui::SameLine();
			ImGui::Checkbox("Surfaces", &connection_surfaces_visible());
			ImGui::SameLine();
			ImGui::Checkbox("Features", &connection_features_visible());
			ImGui::Checkbox("Bounds", &connection_bounds_visible());
			ImGui::SameLine();
			ImGui::Checkbox("Legend", &connection_legend_visible());
			if (connection_legend_visible()) {
				draw_divider();
				draw_connection_legend_chip("motion.rotational", color_for_connection_kind(ConnectionSemanticKind::MotionRotational));
				ImGui::SameLine(0.0f, 12.0f);
				draw_connection_legend_chip("motion.linear", color_for_connection_kind(ConnectionSemanticKind::MotionLinear));
				ImGui::SameLine(0.0f, 12.0f);
				draw_connection_legend_chip("electrical.power", color_for_connection_kind(ConnectionSemanticKind::ElectricalPower));

				draw_connection_legend_chip("electrical.data", color_for_connection_kind(ConnectionSemanticKind::ElectricalData));
				ImGui::SameLine(0.0f, 12.0f);
				draw_connection_legend_chip("temperature.heating", color_for_connection_kind(ConnectionSemanticKind::TemperatureHeating));
				ImGui::SameLine(0.0f, 12.0f);
				draw_connection_legend_chip("temperature.cooling", color_for_connection_kind(ConnectionSemanticKind::TemperatureCooling));

				draw_connection_legend_chip("static.positional", color_for_connection_kind(ConnectionSemanticKind::StaticPositional));
				ImGui::SameLine(0.0f, 12.0f);
				draw_connection_legend_chip("xy plane", color_for_plane("xy"));
				ImGui::SameLine(0.0f, 12.0f);
				draw_connection_legend_chip("xz plane", color_for_plane("xz"));
				ImGui::SameLine(0.0f, 12.0f);
				draw_connection_legend_chip("yz plane", color_for_plane("yz"));

				draw_connection_legend_chip("input", color_for_flow_role(ConnectionFlowRole::Input));
				ImGui::SameLine(0.0f, 12.0f);
				draw_connection_legend_chip("output", color_for_flow_role(ConnectionFlowRole::Output));
				ImGui::SameLine(0.0f, 12.0f);
				draw_connection_legend_chip("bidirectional", color_for_flow_role(ConnectionFlowRole::Bidirectional));
			}
		}
		draw_divider();
		ImGui::Checkbox("AxialProfile sections", &axial_profile_overlay_visible());
		if (axial_profile_overlay_visible()) {
			ImGui::TextColored(color_from_hex(0x66a7ff), "H Hold");
			ImGui::SameLine();
			ImGui::TextColored(color_from_hex(0x66d49b), "L Linear");
			ImGui::SameLine();
			ImGui::TextColored(color_from_hex(0xffa657), "S Step");
		}
		draw_divider();
		ImGui::Checkbox("SMB-OMv2 spatial evidence", &spatial_overlay_visible());
		if (spatial_overlay_visible()) {
			ImGui::Checkbox("Object frame", &spatial_object_frames_visible());
			ImGui::SameLine();
			ImGui::Checkbox("Interfaces", &spatial_interfaces_visible());
			ImGui::SameLine();
			ImGui::Checkbox("AABB", &spatial_bounds_visible());
			ImGui::Checkbox("Connections", &spatial_connections_visible());
			ImGui::SameLine();
			ImGui::Checkbox("Constraints", &spatial_constraints_visible());
			ImGui::Checkbox("Contacts", &spatial_contacts_visible());
			ImGui::SameLine();
			ImGui::Checkbox("Clearances", &spatial_clearances_visible());
			if (current_scene_snapshot() != nullptr &&
			    current_scene_snapshot()->smallModernBuildingModel() != nullptr) {
				ImGui::TextUnformatted("SMB-OMv2.1 knowledge overlays");
				ImGui::Checkbox(
					"Function allocations", &building_function_allocations_visible());
				ImGui::SameLine();
				ImGui::Checkbox("Service flows", &building_service_flows_visible());
				ImGui::Checkbox(
					"Requirement status", &building_requirement_status_visible());
				ImGui::SameLine();
				ImGui::Checkbox(
					"Pending evidence", &building_pending_evidence_visible());
			}
			draw_connection_legend_chip(
				"validated", color_for_spatial_overlay_role(SpatialOverlayColorRole::Validated));
			ImGui::SameLine(0.0f, 10.0f);
			draw_connection_legend_chip(
				"interface", color_for_spatial_overlay_role(SpatialOverlayColorRole::Interface));
			ImGui::SameLine(0.0f, 10.0f);
			draw_connection_legend_chip(
				"connection", color_for_spatial_overlay_role(SpatialOverlayColorRole::Connection));
			draw_connection_legend_chip(
				"constraint", color_for_spatial_overlay_role(SpatialOverlayColorRole::Constraint));
			ImGui::SameLine(0.0f, 10.0f);
			draw_connection_legend_chip(
				"clearance", color_for_spatial_overlay_role(SpatialOverlayColorRole::Clearance));
			ImGui::SameLine(0.0f, 10.0f);
			draw_connection_legend_chip(
				"invalid", color_for_spatial_overlay_role(SpatialOverlayColorRole::Invalid));
		}
		draw_divider();
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0x70808f));
		ImGui::TextUnformatted("Texture mapping");
		ImGui::PopStyleColor();
		int mapping_mode_choice = preview_mapping_mode_override + 1;
		ImGui::SetNextItemWidth(170.0f);
		if (ImGui::Combo("Mode", &mapping_mode_choice, kPreviewMappingLabels, IM_ARRAYSIZE(kPreviewMappingLabels))) {
			preview_mapping_mode_override = mapping_mode_choice - 1;
		}
		int debug_view_choice = static_cast<int>(preview_texture_debug_view);
		ImGui::SetNextItemWidth(170.0f);
		if (ImGui::Combo("Debug", &debug_view_choice, kPreviewDebugLabels, IM_ARRAYSIZE(kPreviewDebugLabels))) {
			preview_texture_debug_view = static_cast<PreviewTextureDebugView>(debug_view_choice);
		}
		ImGui::EndPopup();
	}

	return request_fit_camera;
}

void draw_application_header_content(EditorWorkspaceSession &workspace_session, float header_height)
{
	EditorWorkspaceSession *app_state = &workspace_session;
	begin_surface("HeaderSurface", ImVec2(0.0f, header_height), true, ImVec2(12.0f, 6.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 4.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, 2.0f));
	const std::string document_label = display_document_name(app_state);
	std::string header_status;
	if (scene_generation_in_progress()) {
		header_status = scene_generation_queued() ? "Queued update" : "Regenerating";
	} else if (app_state != nullptr && scene_generation_scheduled()) {
		char status_buffer[64];
		std::snprintf(status_buffer,
		              sizeof(status_buffer),
		              "Auto-run %.1fs",
		              std::max(0.0, scene_generation_scheduled_start_time() - glfwGetTime()));
		header_status = status_buffer;
	}
	if (ImGui::BeginTable("HeaderBar",
	                      3,
	                      ImGuiTableFlags_SizingStretchProp |
	                          ImGuiTableFlags_NoPadOuterX |
	                          ImGuiTableFlags_NoPadInnerX)) {
		ImGui::TableSetupColumn("HeaderIdentity", ImGuiTableColumnFlags_WidthStretch, 0.82f);
		ImGui::TableSetupColumn("HeaderDocument", ImGuiTableColumnFlags_WidthStretch, 1.20f);
		ImGui::TableSetupColumn("HeaderMetrics", ImGuiTableColumnFlags_WidthStretch, 1.58f);

		ImGui::TableNextColumn();
		if (header_logo.texture != 0) {
			const float logo_height = 22.0f;
			const float aspect_ratio =
				header_logo.height > 0 ? static_cast<float>(header_logo.width) / header_logo.height : 1.0f;
			ImGui::Image(reinterpret_cast<ImTextureID>(static_cast<intptr_t>(header_logo.texture)),
			             ImVec2(logo_height * aspect_ratio, logo_height),
			             ImVec2(0.0f, 1.0f),
			             ImVec2(1.0f, 0.0f));
			ImGui::SameLine(0.0f, 8.0f);
		}
		if (app_fonts.title != nullptr) {
			ImGui::PushFont(app_fonts.title);
		}
		ImGui::TextUnformatted("Progen3d");
		if (app_fonts.title != nullptr) {
			ImGui::PopFont();
		}

		ImGui::TableNextColumn();
		draw_status_chip(document_label, 0x6f95ba, 0x436786);
		if (app_state != nullptr && app_state->document.dirty) {
			ImGui::SameLine();
			draw_status_chip("Modified", 0xc8a766, 0x7a6035, 0.14f, 0.26f);
		}

		ImGui::TableNextColumn();
		if (!header_status.empty()) {
			draw_status_chip(header_status, 0x7ea88e, 0x527564, 0.12f, 0.24f);
			ImGui::SameLine();
		}
		draw_status_chip(std::to_string(grammar != nullptr ? grammar->rule_list.size() : 0U) + " rules",
		                 0x8ea5ba,
		                 0x556d83);
		ImGui::SameLine();
		draw_status_chip(std::to_string(grammar != nullptr ? grammar->tokens_new.size() : 0U) + " tokens",
		                 0x8ea5ba,
		                 0x556d83);
			ImGui::SameLine();
			draw_status_chip(std::to_string(active_materials.size()) + " materials",
			                 0x8ea5ba,
			                 0x556d83);
			if (app_state != nullptr && app_state->authentication.session.authenticated) {
				ImGui::SameLine();
				draw_status_chip(app_state->authentication.session.email.empty()
				                     ? std::string("Firebase session")
				                     : app_state->authentication.session.email,
				                 0x7ea88e,
				                 0x527564,
				                 0.12f,
				                 0.24f);
				if (!app_state->authentication.backend_user.role.empty()) {
					ImGui::SameLine();
					draw_status_chip(app_state->authentication.backend_user.role, 0x8ea5ba, 0x556d83);
				}
				if (app_state->authentication.backend_user.credits.loaded) {
					ImGui::SameLine();
					draw_status_chip(backend_credit_plan_label(app_state->authentication.backend_user.credits),
					                 0x93a9bc,
					                 0x5a7187);
					ImGui::SameLine();
					draw_status_chip(backend_credit_available_label(app_state->authentication.backend_user.credits),
					                 0x7ea88e,
					                 0x527564,
					                 0.12f,
					                 0.24f);
					if (app_state->authentication.backend_user.credits.reserved > 0) {
						ImGui::SameLine();
						draw_status_chip(backend_credit_reserved_label(app_state->authentication.backend_user.credits),
						                 0xc8a766,
						                 0x7a6035,
						                 0.14f,
						                 0.26f);
					}
				}
				ImGui::SameLine();
					if (ImGui::Button("Sign Out")) {
						sign_out_firebase(app_state);
					}
				} else {
					ImGui::SameLine();
					if (ImGui::Button("Sign In")) {
						authentication_panel_is_visible() = true;
					}
				}
		ImGui::EndTable();
	}
	ImGui::PopStyleVar(2);
	end_surface();

}

void draw_editor_workspace_content(EditorWorkspaceSession &workspace_session,
	                               float delta_seconds)
{
	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	const float splitter_thickness = 7.0f;

	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
	                                ImGuiWindowFlags_NoMove |
	                                ImGuiWindowFlags_NoSavedSettings;
	ImGui::Begin("Progen3d", nullptr, window_flags);

	if (workspace_session.layout.preview_fullscreen) {
		begin_surface("PreviewRegionFullScreen", ImVec2(0.0f, 0.0f));
		scene_preview_panel.draw(workspace_session, delta_seconds);
		end_surface();
		ImGui::End();
		return;
	}

	application_header_panel.draw(workspace_session);
	if (workspace_session.authentication.session.authenticated &&
	    workspace_session.authentication.status_is_error &&
	    !workspace_session.authentication.status_message.empty()) {
		begin_surface("AuthStatusSurface", ImVec2(0.0f, 0.0f), true, ImVec2(12.0f, 10.0f));
		ImGui::PushStyleColor(ImGuiCol_Text, color_from_hex(0xb85d5d));
		ImGui::TextWrapped("%s", workspace_session.authentication.status_message.c_str());
		ImGui::PopStyleColor();
		end_surface();
		ImGui::Dummy(ImVec2(0.0f, 6.0f));
	}

	ImGui::Dummy(ImVec2(0.0f, 6.0f));
	const ImVec2 content_size = ImGui::GetContentRegionAvail();
	const float total_width = std::max(content_size.x, 720.0f);
	const float total_height = std::max(content_size.y, 320.0f);
	const float min_workspace_width = 360.0f;
	const float min_right_width = 320.0f;
	const float min_preview_height = 220.0f;
	const float min_console_height = 96.0f;

	float workspace_width =
		total_width * std::clamp(workspace_session.layout.editor_width_ratio, 0.28f, 0.76f);
	workspace_width = std::clamp(workspace_width,
	                             min_workspace_width,
	                             total_width - min_right_width - splitter_thickness);
	float right_column_width = total_width - workspace_width - splitter_thickness;

	begin_surface("EditorRegion", ImVec2(workspace_width, total_height));
	draw_editor(&workspace_session);
	end_surface();

	ImGui::SameLine();
	float preview_height =
		total_height * std::clamp(workspace_session.layout.preview_height_ratio, 0.52f, 0.90f);
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
	workspace_session.layout.editor_width_ratio = workspace_width / total_width;

	ImGui::SameLine(0.0f, 0.0f);

	ImGui::BeginChild("RightColumn", ImVec2(0.0f, total_height), false, ImGuiWindowFlags_NoScrollbar);
	begin_surface("PreviewRegion", ImVec2(0.0f, preview_height));
	scene_preview_panel.draw(workspace_session, delta_seconds);
	end_surface();

	draw_splitter("##console_splitter",
	              false,
	              splitter_thickness,
	              std::max(ImGui::GetContentRegionAvail().x, 64.0f),
	              &preview_height,
	              &console_height,
	              min_preview_height,
	              min_console_height);
	workspace_session.layout.preview_height_ratio = preview_height / total_height;

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

void EditorWorkspaceWindow::draw(EditorWorkspaceSession &workspace_session,
	                             float delta_seconds) const
{
	draw_editor_workspace_content(workspace_session, delta_seconds);
}

void CloudDocumentDialog::draw(EditorWorkspaceSession &workspace_session) const
{
	draw_cloud_document_dialog_content(workspace_session);
}

void ApplicationHeaderPanel::draw(EditorWorkspaceSession &workspace_session) const
{
	draw_application_header_content(workspace_session, 48.0f);
}

bool PreviewTimelinePanel::draw(EditorWorkspaceSession &workspace_session,
	                            Context *scene_context,
	                            bool grammar_time_driven,
	                            float current_time) const
{
	return draw_preview_timeline_content(
		workspace_session, scene_context, grammar_time_driven, current_time);
}

void AiGrammarProposalPanel::draw(EditorWorkspaceSession &workspace_session) const
{
	draw_ai_grammar_proposal_content(workspace_session);
}

void AiAssistantPanel::draw(EditorWorkspaceSession &workspace_session) const
{
	draw_ai_assistant_content(workspace_session);
}

void MaterialLibraryPanel::draw(EditorWorkspaceSession &workspace_session) const
{
	draw_material_library_content(workspace_session);
}

void AuthenticationPanel::draw(EditorWorkspaceSession &workspace_session) const
{
	draw_authentication_panel_content(workspace_session);
}

void clear_texture_library_state(EditorWorkspaceSession *app_state)
{
	clear_texture_library_state_internal(app_state);
}

void invalidate_texture_materials(EditorWorkspaceSession *app_state)
{
	invalidate_texture_materials_internal(app_state);
}

void refresh_texture_library(EditorWorkspaceSession *app_state, bool preserve_status_message)
{
	refresh_texture_library_internal(app_state, preserve_status_message);
}

void refresh_ai_threads(EditorWorkspaceSession *app_state, bool preserve_status_message)
{
	refresh_ai_threads_internal(app_state, preserve_status_message);
}

std::string default_new_document_text()
{
	return build_default_new_document_text_internal();
}

void set_editor_document(EditorWorkspaceSession *app_state, const std::string &text, bool dirty)
{
	set_editor_document_internal(app_state, text, dirty);
}

void request_scene_regeneration(const std::string &source_text)
{
	request_scene_regeneration_internal(source_text);
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

Progen3dEditorApplication::Progen3dEditorApplication(ApplicationLaunchOptions launch_options)
	: launch_options_(std::move(launch_options))
{
}

int Progen3dEditorApplication::run()
{
	EditorRuntimeEnvironment runtime_environment;
	runtime_environment.pathLocator().initialize(launch_options_.executablePath().empty()
		                                            ? nullptr
		                                            : launch_options_.executablePath().c_str());

	const uint64_t time_seed =
		static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	grammar_session_seed = time_seed;
	seed_engine(&grammar_rng, grammar_session_seed, 0x4752414d4d415230ULL);
	seed_engine(&effects_rng, grammar_session_seed, 0x4546464543545330ULL); // "EFFECTS0"

	std::string runtime_initialization_error;
	if (!runtime_environment.initialize(1180,
	                                    760,
	                                    "Progen3d",
	                                    glfw_error_callback,
	                                    apply_professional_style,
	                                    &runtime_initialization_error)) {
		errorout(runtime_initialization_error);
		return 1;
	}
	GLFWwindow *window = runtime_environment.window();
	NativeLocalFileDialogService local_file_dialog_service(window);
	std::string local_file_dialog_error;
	if (local_file_dialog_service.initialize(&local_file_dialog_error)) {
		running_local_file_dialog_service = &local_file_dialog_service;
		debugout("Native local file dialog service initialized with the XDG portal backend");
	} else {
		errorout(local_file_dialog_error);
	}
	running_application_window = window;
	application_exit_authorized = false;
	set_window_icon(window,
	                runtime_environment.pathLocator()
		                .resourcePath(std::filesystem::path("assets") / "images" / "progen3d.png")
		                .string());

	EditorWorkspaceSession &app_state = workspace_session_;
	running_workspace_session = &app_state;
	app_state.preview.overlays.spatial_overlay_visible =
		launch_options_.spatialOverlaysRequested() ||
		launch_options_.buildingKnowledgeOverlaysRequested();
	if (launch_options_.buildingKnowledgeOverlaysRequested()) {
		app_state.preview.overlays.spatial_object_frames_visible = false;
		app_state.preview.overlays.spatial_interfaces_visible = false;
		app_state.preview.overlays.spatial_connections_visible = false;
		app_state.preview.overlays.spatial_constraints_visible = false;
		app_state.preview.overlays.spatial_contacts_visible = false;
		app_state.preview.overlays.spatial_clearances_visible = false;
		app_state.preview.overlays.spatial_bounds_visible = false;
		app_state.preview.overlays.building_function_allocations_visible = true;
		app_state.preview.overlays.building_service_flows_visible = true;
		app_state.preview.overlays.building_requirement_status_visible = true;
		app_state.preview.overlays.building_pending_evidence_visible = true;
	}
	app_state.preview.selection.selected_spatial_object_id =
		launch_options_.selectedSpatialObjectId();
	bool startup_document_load_failed = false;
	std::string firebase_init_error;
	if (!initialize_firebase_auth_support(&firebase_init_error) && !firebase_init_error.empty()) {
		debugout("Firebase auth initialization failed: " + firebase_init_error);
	}
	const std::filesystem::path part_class_catalog_path =
		runtime_environment.pathLocator().resourcePath(
			std::filesystem::path("parts") / "part_classes.json");
	const std::filesystem::path header_logo_path =
		runtime_environment.pathLocator().resourcePath(
			std::filesystem::path("assets") / "images" / "pg3d-cubes.png");
	ApplicationStartupProgress startup_progress;

	auto show_startup_loader = [&](const std::string &detail,
	                               float stage_progress = 0.0f,
	                               bool waiting_for_scene = false) {
		float local_progress = std::clamp(stage_progress, 0.0f, 1.0f);
		if (waiting_for_scene) {
			const float pulse = 0.10f *
				(0.5f + 0.5f * std::sin(static_cast<float>(glfwGetTime()) * 3.6f));
			local_progress = std::max(local_progress, 0.58f + pulse);
			local_progress = std::min(local_progress, 0.985f);
		}
		float progress = startup_progress.overallProgress(local_progress);
		if (waiting_for_scene) {
			progress = std::min(progress, 0.985f);
		}
		progress = std::clamp(progress, 0.0f, 1.0f);
		return render_startup_loader_frame(window,
		                                  detail,
		                                  progress,
		                                  startup_progress.completedStageCount(),
		                                  startup_progress.stageCount());
	};

	if (!glfwWindowShouldClose(window)) {
		startup_progress.beginStage(ApplicationStartupStage::PrepareInterface);
		show_startup_loader("Loading interface fonts and preparing the workspace.");
		load_professional_fonts();
		ImGui_ImplOpenGL3_DestroyFontsTexture();
		ImGui_ImplOpenGL3_CreateFontsTexture();
		startup_progress.completeStage(ApplicationStartupStage::PrepareInterface);
	}
	if (!glfwWindowShouldClose(window)) {
		startup_progress.beginStage(ApplicationStartupStage::LoadPartCatalog);
		show_startup_loader("Loading part class definitions.");
		std::string part_catalog_error;
		if (!runtime_environment.partCatalogRepository().loadPartClassCatalog(
				part_class_catalog_path,
				&part_catalog_error)) {
			debugout(part_catalog_error);
		}
		startup_progress.completeStage(ApplicationStartupStage::LoadPartCatalog);
	}
	if (!glfwWindowShouldClose(window)) {
		startup_progress.beginStage(ApplicationStartupStage::LoadInterfaceArtwork);
		show_startup_loader("Loading interface artwork.");
		load_ui_image(header_logo_path.string(), &header_logo);
		startup_progress.completeStage(ApplicationStartupStage::LoadInterfaceArtwork);
	}
	if (!glfwWindowShouldClose(window)) {
		startup_progress.beginStage(ApplicationStartupStage::LoadInitialDocument);
		show_startup_loader(launch_options_.startupDocumentPath().empty()
		                        ? "Creating a new local grammar document."
		                        : "Loading the requested startup grammar document.");
		if (launch_options_.startupDocumentPath().empty()) {
			set_editor_document_internal(&app_state,
			                             build_default_new_document_text_internal(),
			                             false);
		} else {
			DocumentPersistenceService persistence_service;
			LightingScenePersistenceService lighting_persistence_service;
			std::string source_text;
			std::string load_error;
			const std::filesystem::path startup_path = launch_options_.startupDocumentPath();
			if (persistence_service.loadGrammarSource(startup_path,
			                                         &source_text,
			                                         &load_error) &&
			    lighting_persistence_service.load(startup_path,
			                                      &app_state.preview.lighting,
			                                      &load_error)) {
					set_editor_document_internal(&app_state, source_text, false);
					app_state.document.adoptLocalIdentity(startup_path);
					app_state.local_document_workflow.last_successful_directory =
						startup_path.parent_path();
				app_state.local_document_workflow.status_message =
					"Loaded startup document: " + startup_path.string();
				app_state.local_document_workflow.status_is_error = false;
				debugout(app_state.local_document_workflow.status_message);
			} else {
				startup_document_load_failed = true;
				set_editor_document_internal(&app_state,
				                             build_default_new_document_text_internal(),
				                             false);
				app_state.local_document_workflow.status_message = load_error;
				app_state.local_document_workflow.status_is_error = true;
				errorout(load_error);
			}
		}
		startup_progress.completeStage(ApplicationStartupStage::LoadInitialDocument);
	}
	if (!glfwWindowShouldClose(window)) {
		startup_progress.beginStage(ApplicationStartupStage::GenerateInitialScene);
		show_startup_loader("Generating the initial scene preview.");
		regenerate_scene(app_state.document.source_text);
		while (!glfwWindowShouldClose(window)) {
			process_completed_regeneration(&app_state);
			if (!scene_generation_in_progress()) {
				break;
			}
			if (!show_startup_loader("Generating the initial scene preview.", true)) {
				break;
			}
		}
		process_completed_regeneration(&app_state);
		startup_progress.completeStage(ApplicationStartupStage::GenerateInitialScene);
		if (!glfwWindowShouldClose(window)) {
			show_startup_loader("Startup complete.");
		}
	}

	initialize_auth_state(&app_state);
	if (!firebase_init_error.empty()) {
		set_auth_feedback(&app_state, firebase_init_error, true);
	}

	auto render_editor_frame = [&](float delta_seconds) {
		glfwPollEvents();
		process_completed_regeneration(&app_state);
		process_auto_run_timer(&app_state);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		if (ImGui::IsKeyPressed(ImGuiKey_F11, false)) {
			app_state.layout.preview_fullscreen = !app_state.layout.preview_fullscreen;
		}

		editor_workspace_window.draw(app_state, delta_seconds);
		if (authentication_panel_is_visible()) {
			authentication_panel.draw(app_state);
		}

		ImGui::Render();
		int display_width = 0;
		int display_height = 0;
		glfwGetFramebufferSize(window, &display_width, &display_height);
		glViewport(0, 0, display_width, display_height);
		glClearColor(0.972f, 0.957f, 0.933f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			glfwSwapBuffers(window);
			process_pending_local_file_dialog(&app_state);
		};

	int application_exit_code = 0;
	if (launch_options_.smokeTestRequested()) {
		if (startup_document_load_failed) {
			errorout("PROGEN3D_GUI_SMOKE_TEST_FAILED: startup document could not be loaded");
			application_exit_code = 2;
		} else if (current_scene_snapshot() == nullptr ||
		           current_scene_snapshot()->generationContext() == nullptr ||
		           grammar == nullptr ||
		           !grammar->error_details.empty()) {
			errorout("PROGEN3D_GUI_SMOKE_TEST_FAILED: initial scene generation did not produce a valid context");
			application_exit_code = 2;
		}

		if (application_exit_code == 0 && launch_options_.temporalSmokeTestRequested()) {
			if (!grammarUsesTime(grammar)) {
				errorout("PROGEN3D_GUI_TEMPORAL_SMOKE_TEST_FAILED: startup grammar does not use t");
				application_exit_code = 3;
			} else {
				constexpr double kTemporalSmokeSampleTime = 0.75;
				const std::uint64_t expected_design_nonce = current_scene_snapshot()->designNonce();
				request_scene_time_sample_internal(app_state.document.source_text,
				                                   kTemporalSmokeSampleTime);
				const double sample_deadline = glfwGetTime() + 20.0;
				while (!glfwWindowShouldClose(window) && scene_generation_in_progress() &&
				       glfwGetTime() < sample_deadline) {
					process_completed_regeneration(&app_state);
					if (!scene_generation_in_progress()) {
						break;
					}
					if (!show_startup_loader("Sampling grammar time for verification.", true)) {
						break;
					}
				}
				process_completed_regeneration(&app_state);
				if (current_scene_snapshot() == nullptr ||
				    current_scene_snapshot()->generationContext() == nullptr ||
				    current_scene_snapshot()->designNonce() != expected_design_nonce ||
				    std::fabs(current_scene_snapshot()->evaluationTime() - kTemporalSmokeSampleTime) >
				        0.000001) {
					errorout("PROGEN3D_GUI_TEMPORAL_SMOKE_TEST_FAILED: grammar-time sample was not published deterministically");
					application_exit_code = 3;
				} else {
					debugout("PROGEN3D_GUI_TEMPORAL_SMOKE_TEST_PASSED");
				}
			}
		}

		if (application_exit_code == 0) {
			const bool document_dirty_before_fov_check = app_state.document.dirty;
			const std::uint64_t generation_request_before_fov_check =
				app_state.document.last_successful_generation_request_id;
			for (const float field_of_view_degrees :
			     std::array<float, 3>{24.0f, 43.0f, 70.0f}) {
				app_state.preview.projection.setVerticalFieldOfViewDegrees(
					field_of_view_degrees);
				render_editor_frame(1.0f / 60.0f);
				if (preview_texture_id() == 0 ||
				    current_preview_session().statistics.primitive_count == 0 ||
				    std::fabs(app_state.preview.projection.verticalFieldOfViewDegrees() -
				              field_of_view_degrees) > 0.0001f) {
					errorout(
						"PROGEN3D_GUI_FOV_SMOKE_TEST_FAILED: preview did not render a requested field of view");
					application_exit_code = 2;
					break;
				}
			}
			if (application_exit_code == 0 &&
			    (app_state.document.dirty != document_dirty_before_fov_check ||
			     app_state.document.last_successful_generation_request_id !=
			         generation_request_before_fov_check ||
			     scene_generation_in_progress())) {
				errorout(
					"PROGEN3D_GUI_FOV_SMOKE_TEST_FAILED: camera lens changes affected document or generation state");
				application_exit_code = 2;
			}
			app_state.preview.projection.reset();
			if (application_exit_code == 0) {
				render_editor_frame(1.0f / 60.0f);
			}
				if (application_exit_code == 0) {
					debugout("PROGEN3D_GUI_FOV_SMOKE_TEST_PASSED");
					debugout("PROGEN3D_GUI_RENDER_FRAME_PASSED");
				}
			}

			if (application_exit_code == 0 &&
			    !launch_options_.previewHiddenObjectIds().empty()) {
				const bool document_dirty_before_object_exclusion = app_state.document.dirty;
				const std::uint64_t generation_request_before_object_exclusion =
					app_state.document.last_successful_generation_request_id;
				const PreviewObjectExclusionEvidence exclusion_evidence =
					exclude_spatial_objects_from_preview(
						launch_options_.previewHiddenObjectIds());
				if (exclusion_evidence.matched_object_count !=
				        exclusion_evidence.requested_object_count ||
				    exclusion_evidence.hidden_primitive_count == 0) {
					errorout(
						"PROGEN3D_GUI_HIDDEN_OBJECTS_FAILED: one or more requested spatial objects had no preview geometry");
					application_exit_code = 4;
				} else {
					upload_context_preview_buffers(
						&app_state,
						current_scene_snapshot()->generationContext());
					render_editor_frame(1.0f / 60.0f);
				}
				if (application_exit_code == 0 &&
				    (app_state.document.dirty != document_dirty_before_object_exclusion ||
				     app_state.document.last_successful_generation_request_id !=
				         generation_request_before_object_exclusion ||
				     scene_generation_in_progress())) {
					errorout(
						"PROGEN3D_GUI_HIDDEN_OBJECTS_FAILED: preview-only visibility affected document or generation state");
					application_exit_code = 4;
				}
				if (application_exit_code == 0) {
					std::ostringstream message;
					message << "PROGEN3D_GUI_HIDDEN_OBJECTS_PASSED objects="
					        << exclusion_evidence.matched_object_count
					        << " primitives="
					        << exclusion_evidence.hidden_primitive_count;
					debugout(message.str());
				}
			}

			const PreviewViewSelection requested_preview_view =
				launch_options_.previewViewSelection();
			const std::optional<PreviewOrientation> requested_camera_orientation =
				camera_orientation_for_preview_view(requested_preview_view);
			if (application_exit_code == 0 &&
			    (requested_camera_orientation.has_value() ||
			     launch_options_.previewFitExtentsRequested())) {
				const bool document_dirty_before_camera_framing = app_state.document.dirty;
				const std::uint64_t generation_request_before_camera_framing =
					app_state.document.last_successful_generation_request_id;
				if (requested_camera_orientation.has_value()) {
					current_preview_session().camera.orientTo(*requested_camera_orientation);
				}
				if (launch_options_.previewFitExtentsRequested()) {
					current_preview_session().fit_camera_pending = true;
				}
				render_editor_frame(1.0f / 60.0f);
				if (launch_options_.previewFitExtentsRequested()) {
					current_preview_session().camera.zoomByWheel(1.5f, 1.0f);
					render_editor_frame(1.0f / 60.0f);
				}
				if (app_state.document.dirty != document_dirty_before_camera_framing ||
				    app_state.document.last_successful_generation_request_id !=
				        generation_request_before_camera_framing ||
				    scene_generation_in_progress()) {
					errorout(
						"PROGEN3D_GUI_CAMERA_FRAMING_FAILED: preview framing affected document or generation state");
					application_exit_code = 4;
				}
				if (application_exit_code == 0 && requested_camera_orientation.has_value()) {
					std::ostringstream message;
					message << "PROGEN3D_GUI_VIEW_PASSED name="
					        << preview_view_selection_name(requested_preview_view)
					        << " azimuth="
					        << current_preview_session().camera.azimuthDegrees()
					        << " elevation="
					        << current_preview_session().camera.elevationDegrees();
					debugout(message.str());
				}
				if (application_exit_code == 0 &&
				    launch_options_.previewIsometricRequested()) {
					std::ostringstream message;
					message << "PROGEN3D_GUI_ISOMETRIC_VIEW_PASSED azimuth="
					        << current_preview_session().camera.azimuthDegrees()
					        << " elevation="
					        << current_preview_session().camera.elevationDegrees();
					debugout(message.str());
				}
				if (application_exit_code == 0 &&
				    launch_options_.previewFitExtentsRequested()) {
					std::ostringstream message;
					message << "PROGEN3D_GUI_FIT_EXTENTS_PASSED distance="
					        << current_preview_session().camera.distance();
					debugout(message.str());
				}
			}

			if (application_exit_code == 0 &&
			    launch_options_.buildingKnowledgeOverlaysRequested()) {
				const GeneratedSceneSnapshot *snapshot = current_scene_snapshot().get();
				const SmallModernBuildingModel *building_model =
					snapshot != nullptr ? snapshot->smallModernBuildingModel() : nullptr;
				const std::string &selected_object_id =
					launch_options_.selectedSpatialObjectId();
				const BuildingObjectSemanticProfile *profile =
					building_model != nullptr && !selected_object_id.empty()
						? building_model->classificationModel().findProfile(
							SpatialObjectId(selected_object_id))
						: nullptr;
				if (building_model == nullptr || profile == nullptr) {
					errorout(
						"PROGEN3D_GUI_BUILDING_KNOWLEDGE_OVERLAY_FAILED: selected object has no published SMB-OMv2.1 profile");
					application_exit_code = 4;
				} else {
					std::ostringstream message;
					message << "PROGEN3D_GUI_BUILDING_KNOWLEDGE_OVERLAY_PASSED object="
					        << selected_object_id << " profile_hash=" << std::hex
					        << profile->profileHash() << " model_hash="
					        << building_model->modelHash();
					debugout(message.str());
				}
			}

			if (application_exit_code == 0 && launch_options_.visualTestRequested()) {
			if (launch_options_.previewCapturePath().empty()) {
				errorout(
					"PROGEN3D_GUI_VISUAL_TEST_FAILED: --visual-test requires --capture-preview <path>");
				application_exit_code = 4;
			} else {
				const PreviewCaptureEvidence capture_evidence =
					PreviewFramebufferCaptureService().captureCurrentPreview(
						launch_options_.previewCapturePath());
				if (!capture_evidence.succeeded()) {
					errorout(
						"PROGEN3D_GUI_VISUAL_TEST_FAILED: " + capture_evidence.message());
					application_exit_code = 4;
				} else {
					std::ostringstream message;
					message << "PROGEN3D_GUI_VISUAL_CAPTURE_PASSED path="
					        << capture_evidence.outputPath()
					        << " width=" << capture_evidence.width()
					        << " height=" << capture_evidence.height()
					        << " pixel_hash=" << capture_evidence.pixelHash();
					debugout(message.str());
				}
			}
		}

		if (application_exit_code == 0) {
			debugout("PROGEN3D_GUI_SMOKE_TEST_PASSED");
		}
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}

	if (!launch_options_.smokeTestRequested()) {
		double last_time = glfwGetTime();
		while (true) {
			if (glfwWindowShouldClose(window)) {
				if (application_exit_authorized) {
					break;
				}
				glfwSetWindowShouldClose(window, GLFW_FALSE);
				request_document_replacement(&app_state,
				                             DocumentReplacementAction::ExitApplication);
			}

			const double now = glfwGetTime();
			const float delta_seconds = static_cast<float>(now - last_time);
			last_time = now;
			render_editor_frame(delta_seconds);
		}
	}

	cancel_cloud_dialog_async_requests(&app_state);
	if (scene_regeneration_coordinator != nullptr) {
		scene_regeneration_coordinator->shutdown();
		scene_regeneration_coordinator.reset();
	}
	grammar_compilation_service.reset();
	destroy_grammar();
	clear_variable_pool();
	destroy_ui_image(&header_logo);
	destroy_materials();
	shutdown_firebase_auth_support();
	running_local_file_dialog_service = nullptr;
	running_application_window = nullptr;
	local_file_dialog_service.shutdown();
	runtime_environment.shutdown();
	running_workspace_session = nullptr;
	return application_exit_code;
}
