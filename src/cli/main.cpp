#include "CalibratedCamera.h"
#include "DepthOfField.h"
#include "GrammarImageFit.h"
#include "ManifestInput.h"
#include "VideoComposition.h"
#include "VideoEdit.h"
#include "FrameOutput.h"
#include "FlowOverlay.h"
#include "RenderCheckpoint.h"
#include "RenderExecution.h"
#include "RenderEffectInputs.h"
#include "RenderFiles.h"
#include "RenderQueue.h"
#include "RenderSweep.h"
#include "ShotCamera.h"
#include "ShutterExposure.h"
#include "RenderSceneTimeline.h"
#include "SequenceAudioCues.h"
#include "SequenceOutput.h"
#include "TimelineSettings.h"

#include "AppPaths.h"
#include "Context.h"
#include "editor/controller/PreviewCameraController.h"
#include "editor/model/PreviewImage.h"
#include "editor/model/RenderConfiguration.h"
#include "editor/service/GrammarCompilationService.h"
#include "editor/service/GrammarStructureValidationService.h"
#include "editor/service/PreviewFramebufferCaptureService.h"
#include "editor/service/PreviewGeometryUploadService.h"
#include "editor/service/PreviewMaterialLibrary.h"
#include "electrical/service/LightingStateEvaluator.h"
#include "lighting/model/LightingSceneDefinition.h"
#include "physics/model/SimulationStep.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <json/json.h>
#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <set>
#include <optional>
#include <random>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>

std::mt19937_64 grammar_rng;
std::mt19937_64 effects_rng;

namespace {

namespace fs = std::filesystem;
constexpr int schema_version = 1;
constexpr int maximum_dimension = 4096;
constexpr std::int64_t maximum_pixel_count = 8 * 1024 * 1024;
constexpr const char *usage = "progen3d-cli doctor | validate <job.json> | "
                              "render <job.json> [--out <directory> [--resume]] | compose <edit.json> --out <video.mp4> | "
                              "enqueue <job.json> [--queue <directory>] | status [<id>] [--queue <directory>] | "
                              "cancel <id> [--queue <directory>] | resume <id> [--queue <directory>] | "
                              "worker [--queue <directory>] [--once] | sweep <experiment.json> --out <directory> | "
                              "fit <grammar> --views <references.json> --out <directory>";

enum class ExitCode : int
{
	success = 0,
	invalid_job = 2,
	grammar_failure = 3,
	render_failure = 4,
	output_failure = 5,
	cancelled = render_cancelled_exit_code
};

class JobFailure : public std::runtime_error
{
public:
	JobFailure(ExitCode code, const std::string &message)
		: std::runtime_error(message), code_(code)
	{
	}

	ExitCode code() const { return code_; }

private:
	ExitCode code_;
};

std::vector<std::string> engine_errors;

ExitCode fail(Json::Value &report, ExitCode code, const std::string &message)
{
	report["error"] = message;
	return code;
}

void add_unique_diagnostic(Json::Value &list, const std::string &message)
{
	const Json::Value entry(message);
	if (std::find(list.begin(), list.end(), entry) == list.end()) {
		list.append(entry);
	}
}

using manifest_input::is_unsigned_integer;

int read_dimension(const Json::Value &value, const char *field)
{
	if (!is_unsigned_integer(value) || value.asUInt64() < 1 || value.asUInt64() > maximum_dimension) {
		throw std::runtime_error(std::string(field) + " must be an integer from 1 to 4096.");
	}
	return value.asInt();
}

class RenderJobManifest
{
public:
	explicit RenderJobManifest(const fs::path &manifest_path)
		: manifest(fs::absolute(manifest_path).lexically_normal())
	{
		Json::Value job = manifest_input::read_json(manifest);
		if (!job.isObject() || !is_unsigned_integer(job["schema_version"]) ||
		    job["schema_version"].asUInt64() != schema_version) {
			throw std::runtime_error("The job must be a JSON object with schema_version set to 1.");
		}
		for (const std::string &field : job.getMemberNames()) {
			if (field != "schema_version" && field != "grammar" && field != "seed" &&
			    field != "width" && field != "height" && field != "output_dir" && field != "strict" &&
			    field != "camera" && field != "orthographic" && field != "layers" && field != "timeline" &&
			    field != "overlays" && field != "overlay_manifest" && field != "input_sha256") {
				throw std::runtime_error("Unknown job field: " + field);
			}
		}
		resolve_render_effect_inputs(job, manifest.parent_path());
		if (job.isMember("overlays")) overlay = FlowOverlay::parse(job["overlays"]);
		grammar = manifest_input::resolve_path(manifest.parent_path(), job["grammar"], "grammar");
		output_directory = manifest_input::resolve_path(manifest.parent_path(), job["output_dir"], "output_dir");
		if (!is_unsigned_integer(job["seed"])) {
			throw std::runtime_error("seed must be an unsigned 64-bit integer.");
		}
		seed = job["seed"].asUInt64();
		width = read_dimension(job["width"], "width");
		height = read_dimension(job["height"], "height");
		if (static_cast<std::int64_t>(width) * height > maximum_pixel_count) {
			throw std::runtime_error("width times height must be at most 8388608 pixels.");
		}
		if (job.isMember("strict")) {
			if (!job["strict"].isBool()) {
				throw std::runtime_error("strict must be a boolean.");
			}
			strict = job["strict"].asBool();
		}
		if (job.isMember("camera")) {
			camera = ShotCamera::parse(job["camera"]);
			export_layers = true;
		}
		if (job.isMember("orthographic")) {
			if (camera) throw std::runtime_error("Choose either camera or orthographic calibration in a render job.");
			orthographic = CalibratedCamera::parse(job["orthographic"], width, height);
		}
		if (job.isMember("layers")) {
			if (!job["layers"].isBool()) {
				throw std::runtime_error("layers must be a boolean.");
			}
			export_layers = job["layers"].asBool();
		}
		if (job.isMember("timeline")) {
			timeline = TimelineSettings::parse(job["timeline"]);
		}
		snapshot_ = job;
		snapshot_["grammar"] = grammar.u8string();
		snapshot_["output_dir"] = output_directory.u8string();
	}

	const Json::Value &snapshot() const { return snapshot_; }
	std::uint32_t totalFrames() const { return timeline ? timeline->end_frame - timeline->start_frame : 1u; }

	const fs::path manifest;
	fs::path grammar;
	fs::path output_directory;
	std::uint64_t seed = 0;
	int width = 0;
	int height = 0;
	bool strict = true;
	bool export_layers = false;
	std::optional<ShotCamera> camera;
	std::optional<FlowOverlay> overlay;
	std::optional<CalibratedCamera> orthographic;
	std::optional<TimelineSettings> timeline;

private:
	Json::Value snapshot_;
};

std::string describe_glfw_error(const char *message)
{
	const char *detail = nullptr;
	glfwGetError(&detail);
	return detail == nullptr ? message : std::string(message) + ": " + detail;
}

class OffscreenRenderContext
{
public:
	OffscreenRenderContext() = default;
	OffscreenRenderContext(const OffscreenRenderContext &) = delete;
	OffscreenRenderContext &operator=(const OffscreenRenderContext &) = delete;

	~OffscreenRenderContext()
	{
		if (renderer_started_) {
			preview_renderer().shutdown();
		}
		if (window_ != nullptr) {
			glfwDestroyWindow(window_);
		}
		if (glfw_started_) {
			glfwTerminate();
		}
	}

	void initialize(Json::Value &report)
	{
		if (glfwInit() != GLFW_TRUE) {
			throw std::runtime_error(describe_glfw_error("Cannot initialize GLFW in this Windows session"));
		}
		glfw_started_ = true;
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		window_ = glfwCreateWindow(1, 1, "ProGen3D offscreen", nullptr, nullptr);
		if (window_ == nullptr) {
			throw std::runtime_error(describe_glfw_error(
				"Cannot create a hidden OpenGL 4.6 context. Use a GPU-enabled Windows desktop session"));
		}
		glfwMakeContextCurrent(window_);
		if (!progen3d_initialize_gl_loader(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress))) {
			throw std::runtime_error("Cannot load OpenGL functions.");
		}
		report["opengl"]["vendor"] = gl_string(GL_VENDOR);
		report["opengl"]["renderer"] = gl_string(GL_RENDERER);
		report["opengl"]["version"] = gl_string(GL_VERSION);

		renderer_started_ = true;
		if (!preview_renderer().initialize()) {
			throw std::runtime_error("Cannot initialize the OpenGL preview renderer.");
		}
	}

private:
	static const char *gl_string(GLenum name)
	{
		const auto *value = glGetString(name);
		return value == nullptr ? "" : reinterpret_cast<const char *>(value);
	}

	bool glfw_started_ = false;
	bool renderer_started_ = false;
	GLFWwindow *window_ = nullptr;
};

class OffscreenPreviewMaterials
{
public:
	OffscreenPreviewMaterials() = default;
	OffscreenPreviewMaterials(const OffscreenPreviewMaterials &) = delete;
	OffscreenPreviewMaterials &operator=(const OffscreenPreviewMaterials &) = delete;

	~OffscreenPreviewMaterials()
	{
		for (auto &entry : cache_) {
			preview_materials::destroy_preview_material_textures(&entry.second);
		}
	}

	const std::vector<PreviewMaterial> &load(const std::vector<std::string> &names, Json::Value &warnings)
	{
		for (auto entry = cache_.begin(); entry != cache_.end();) {
			if (std::find(names.begin(), names.end(), entry->first) == names.end()) {
				preview_materials::destroy_preview_material_textures(&entry->second);
				entry = cache_.erase(entry);
			} else {
				++entry;
			}
		}
		const RenderConfiguration defaults;
		materials_.clear();
		materials_.reserve(names.size());
		for (const std::string &name : names) {
			auto material = cache_.find(name);
			if (material == cache_.end()) {
				const bool user_texture = preview_materials::is_user_texture_slot_name(name);
				if (user_texture) {
					add_unique_diagnostic(warnings, "Using a neutral material for " + name +
					                              " because user texture slots require the editor.");
				}
				auto generated = preview_materials::generate_procedural_material(
					user_texture ? "softwhitematteplaster" : name, defaults.material_texture_resolution);
				material = cache_.emplace(name, std::move(generated)).first;
			}
			materials_.push_back(material->second);
		}
		return materials_;
	}

private:
	std::unordered_map<std::string, PreviewMaterial> cache_;
	std::vector<PreviewMaterial> materials_;
};

Json::Value format_grammar_diagnostics(const std::vector<GrammarDiagnostic> &diagnostics)
{
	Json::Value result(Json::arrayValue);
	for (const GrammarDiagnostic &diagnostic : diagnostics) {
		Json::Value item(Json::objectValue);
		item["message"] = diagnostic.message;
		if (diagnostic.line >= 0) {
			item["line"] = diagnostic.line + 1;
			item["column"] = diagnostic.start_column + 1;
			item["end_column_exclusive"] = diagnostic.end_column + 1;
		}
		result.append(std::move(item));
	}
	return result;
}

SceneGenerationRequest scene_request(const RenderJobManifest &job, const fs::path &grammar_snapshot)
{
	SceneGenerationRequest request;
	request.source_text = render_files::read_text(grammar_snapshot.empty() ? job.grammar : grammar_snapshot);
	request.source_name = job.grammar.u8string();
	request.design_nonce = job.seed;
	return request;
}

std::unique_ptr<SceneGenerationResult> compile_job(const SceneGenerationRequest &request)
{
	engine_errors.clear();
	const GrammarCompilationService compiler(
		[](const std::string &, std::uint64_t seed) {
			grammar_rng.seed(seed);
			effects_rng.seed(seed ^ 0x4546464543545330ULL);
		},
		[](GrammarDocument &grammar, bool) {
			return GrammarStructureValidationService::validate(&grammar, true);
		});
	return compiler.compile(request);
}

std::string generation_error(const SceneGenerationResult &compiled, bool strict, Json::Value &report)
{
	report["diagnostics"] = format_grammar_diagnostics(compiled.diagnostics);
	bool runtime_failed = false;
	for (const std::string &message : engine_errors) {
		if (message.rfind("Grammar execution warning:", 0) == 0) {
			add_unique_diagnostic(report["warnings"], message);
		} else {
			add_unique_diagnostic(report["runtime_errors"], message);
			runtime_failed = true;
		}
	}
	if (!compiled.succeeded()) {
		return compiled.error.empty() ? "Grammar compilation failed." : compiled.error;
	}
	if (strict && runtime_failed) {
		return "Grammar execution reported errors.";
	}
	if (runtime_failed) {
		add_unique_diagnostic(report["warnings"], "Permissive mode accepted partial geometry after grammar execution errors.");
	}
	return {};
}

PreviewCameraState frame_scene(const PrimitiveBounds &bounds, int width, int height)
{
	const PreviewProjectionConfiguration projection;
	PreviewCameraController camera;
	camera.orientTo(PreviewOrientation::Isometric);
	camera.fitToBounds(bounds.center.x, bounds.center.y, bounds.center.z,
	                   bounds.half_extents.x, bounds.half_extents.y, bounds.half_extents.z,
	                   static_cast<float>(width), static_cast<float>(height), projection);
	PreviewCameraState state;
	state.width = width;
	state.height = height;
	state.camera_distance = camera.distance();
	state.scale_global = camera.scaleFactor();
	state.angle_view = camera.azimuthDegrees();
	state.elevation_view = camera.elevationDegrees();
	state.roll_view = camera.rollDegrees();
	state.target = {camera.targetX(), camera.targetY(), camera.targetZ()};
	state.vertical_field_of_view_radians = projection.verticalFieldOfViewRadians();
	return state;
}

PreviewImage read_preview(const PreviewCameraState &camera)
{
	std::vector<std::uint8_t> pixels;
	int width = 0;
	int height = 0;
	if (!read_preview_rgb_pixels(&pixels, &width, &height) ||
	    width != camera.width || height != camera.height || glGetError() != GL_NO_ERROR) {
		throw std::runtime_error("Cannot read the OpenGL preview framebuffer at the requested size.");
	}
	PreviewImage image(width, height, std::move(pixels));
	if (!image.isValid()) {
		throw std::runtime_error("The OpenGL preview framebuffer returned incomplete pixels.");
	}
	return image;
}

std::size_t count_changed_pixels(const PreviewImage &background, const PreviewImage &render)
{
	std::size_t count = 0;
	const auto &background_pixels = background.rgbPixels();
	const auto &render_pixels = render.rgbPixels();
	for (std::size_t offset = 0; offset < render_pixels.size(); offset += 3) {
		if (background_pixels[offset] != render_pixels[offset] ||
		    background_pixels[offset + 1] != render_pixels[offset + 1] ||
		    background_pixels[offset + 2] != render_pixels[offset + 2]) {
			++count;
		}
	}
	return count;
}

PreviewCameraState frame_shot(const ShotCameraSample &shot, const PrimitiveBounds &bounds,
                              int width, int height)
{
	PreviewCameraState camera;
	camera.width = width;
	camera.height = height;
	camera.world_position = shot.position;
	camera.world_up = shot.world_up;
	camera.roll_view = shot.roll_degrees;
	camera.target = shot.aim;
	camera.vertical_field_of_view_radians = glm::radians(shot.lens.fov_degrees);
	const glm::dvec3 forward = glm::normalize(glm::dvec3(shot.aim) - glm::dvec3(shot.position));
	const double scene_far = glm::dot(glm::dvec3(bounds.center) - glm::dvec3(shot.position), forward) +
	                         glm::dot(glm::abs(forward), glm::dvec3(bounds.half_extents));
	if (!std::isfinite(scene_far) || scene_far > std::numeric_limits<float>::max() / 2.0) {
		throw std::runtime_error("The scene bounds exceed the camera's depth range.");
	}
	camera.far_clip = static_cast<float>(std::max(100.0, scene_far + 10.0));
	return camera;
}

Json::Value projection_report(const PreviewCameraState &camera)
{
	const auto matrix_json = [](const glm::mat4 &matrix) {
		Json::Value result(Json::arrayValue);
		for (int column = 0; column < 4; ++column) {
			for (int row = 0; row < 4; ++row) result.append(matrix[column][row]);
		}
		return result;
	};
	const auto matrices = preview_camera_matrices(camera);
	Json::Value report;
	report["view_column_major"] = matrix_json(matrices.view);
	report["projection_column_major"] = matrix_json(matrices.projection);
	report["world_scale"] = camera.scale_global;
	report["width"] = camera.width;
	report["height"] = camera.height;
	return report;
}

Json::Value camera_report(const ShotCameraSample &camera, const PreviewCameraState *projection = nullptr)
{
	const auto vector = [](const glm::vec3 &point) {
		Json::Value value(Json::arrayValue);
		value.append(point.x);
		value.append(point.y);
		value.append(point.z);
		return value;
	};
	Json::Value report;
	report["time_seconds"] = camera.time_seconds;
	report["position"] = vector(camera.position);
	report["aim"] = vector(camera.aim);
	report["focus"] = vector(camera.focus);
	report["focus_depth"] = camera.focus_depth;
	report["fov_degrees"] = camera.lens.fov_degrees;
	report["aperture_radius"] = camera.lens.aperture_radius;
	report["max_blur_px"] = camera.lens.max_blur_px;
	report["roll_degrees"] = camera.roll_degrees;
	if (camera.world_up) report["world_up"] = vector(*camera.world_up);
	if (camera.rail) {
		report["rail"]["id"] = camera.rail->id;
		report["rail"]["distance"] = camera.rail->distance;
		report["rail"]["speed"] = camera.rail->speed;
	}
	if (projection) report["projection"] = projection_report(*projection);
	return report;
}

void include_emitter_depth(std::vector<float> &depth, const std::vector<float> &emitter_depth)
{
	for (std::size_t pixel = 0; pixel < emitter_depth.size(); ++pixel) {
		if (emitter_depth[pixel] > 0.0f && (depth[pixel] <= 0.0f || emitter_depth[pixel] < depth[pixel])) {
			depth[pixel] = emitter_depth[pixel];
		}
	}
}

DepthOfFieldResult composite_shot(const DepthOfField &lens, PreviewRenderer &renderer,
                                  const SceneLayerCapture &layer_capture, const PreviewCameraState &camera,
                                  const PreviewRenderSettings &settings, const std::vector<PreviewMaterial> &materials,
                                  const PreviewLightCollection &lights, const PreviewImage &sharp,
                                  const std::vector<float> &sharp_depth, const PreviewImage &background,
                                  const FlowOverlay *overlay, const SceneGenerationContext &scene, double time)
{
	const float cutoff = lens.foregroundCutoff();
	const bool has_foreground = std::any_of(sharp_depth.begin(), sharp_depth.end(),
		[cutoff](float depth) { return depth > 0.0f && depth < cutoff; });
	if (!has_foreground) {
		return lens.composite(sharp, sharp_depth, sharp, sharp_depth);
	}
	if (cutoff >= camera.far_clip) {
		const std::vector<float> sky_depth(sharp_depth.size(), 0.0f);
		return lens.composite(sharp, sharp_depth, background, sky_depth);
	}

	// Clip the foreground out of a second colour and depth pass to reveal surfaces
	// behind softened silhouettes. A projection near plane keeps the framing fixed.
	PreviewCameraState far_camera = camera;
	far_camera.near_clip = cutoff;
	renderer.render(far_camera, settings, materials, lights);
	PreviewImage far_plate = read_preview(far_camera);
	auto far_layers = layer_capture.capture(far_camera, false);
	if (overlay) {
		const auto effects = overlay->render(scene, far_camera, time, far_layers.linear_depth);
		far_plate = effects.composite(far_plate);
		include_emitter_depth(far_layers.linear_depth, effects.depth);
	}
	return lens.composite(sharp, sharp_depth, far_plate, far_layers.linear_depth);
}

class OffscreenFrameRenderer
{
public:
	OffscreenFrameRenderer(int width, int height, Json::Value &report) : width_(width), height_(height)
	{
		context_.initialize(report);
		GLint max_texture = 0;
		GLint max_renderbuffer = 0;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_texture);
		glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &max_renderbuffer);
		if (std::max(width_, height_) > std::min(max_texture, max_renderbuffer)) {
			throw std::runtime_error("The requested dimensions exceed the OpenGL framebuffer limit.");
		}
	}

	RenderedFrame render(const SceneGenerationContext &scene, const std::optional<ShotCameraSample> &shot,
	                     bool export_layers, double sample_time, Json::Value &report,
	                     const PreviewCameraState *sequence_camera = nullptr, bool suppress_grid = false,
	                     const FlowOverlay *overlay = nullptr, bool static_geometry = false);

private:
	OffscreenRenderContext context_;
	OffscreenPreviewMaterials materials_;
	SceneLayerCapture layer_capture_;
	std::optional<PreviewImage> background_;
	bool background_has_grid_ = false;
	bool has_geometry_ = false;
	bool has_layer_geometry_ = false;
	int width_;
	int height_;
};

RenderedFrame OffscreenFrameRenderer::render(const SceneGenerationContext &scene,
                                             const std::optional<ShotCameraSample> &shot,
                                             bool export_layers, double sample_time, Json::Value &report,
                                             const PreviewCameraState *sequence_camera, bool suppress_grid,
                                             const FlowOverlay *overlay, bool static_geometry)
{
	const PrimitiveBounds bounds = scene.getSceneBounds();
	if (!bounds.valid && sequence_camera == nullptr) {
		throw std::runtime_error("The grammar generated no drawable scene bounds.");
	}

	const PreviewCameraState fitted = sequence_camera == nullptr
		? frame_scene(bounds, width_, height_) : *sequence_camera;
	const PreviewCameraState camera = shot ? frame_shot(*shot, bounds, width_, height_) : fitted;
	PreviewRenderSettings settings;
	settings.show_grid = !shot && !export_layers && !suppress_grid;
	settings.preview_time = static_cast<float>(sample_time);
	PreviewLightCollection lights;
	lights.loadStudioDefault();
	const LightingSceneDefinition &lighting = scene.lightingSceneDefinition();
	lights.replaceGrammarLights(lighting.lights());
	LightingStateEvaluator().apply(lighting.electricalControlGraph(), &lights);

	PreviewRenderer &renderer = preview_renderer();
	// The studio background is screen-space. The grid uses this sample's camera.
	if (!background_ || settings.show_grid || background_has_grid_) {
		renderer.render(camera, settings, {}, lights);
		background_ = read_preview(camera);
		background_has_grid_ = settings.show_grid;
	}
	const PreviewImage &background = *background_;
	const auto &materials = materials_.load(scene.getMaterialNames(), report["warnings"]);
	if (!static_geometry || !has_geometry_) {
		PreviewGeometryUploadService::uploadStatic(&scene, materials);
		PreviewGeometryUploadService::uploadDynamic(&scene, materials.size());
		has_geometry_ = true;
	}
	renderer.render(camera, settings, materials, lights);

	PreviewImage sharp = read_preview(camera);
	const std::size_t changed_pixels = count_changed_pixels(background, sharp);
	report["pixels_different_from_background"] = static_cast<Json::UInt64>(changed_pixels);
	if (changed_pixels == 0 && sequence_camera == nullptr) {
		throw std::runtime_error("The preview is blank. No pixels differ from the background-only frame.");
	}
	const auto lens = shot ? std::optional<DepthOfField>(std::in_place, *shot, height_) : std::nullopt;
	const bool blur = lens && lens->enabled();
	const bool active_overlay = overlay && overlay->active(sample_time);
	std::optional<RenderFrameOverlay> evidence;
	if (overlay) {
		evidence.emplace();
		evidence->records["schema_version"] = 1;
		evidence->records["time_seconds"] = sample_time;
		evidence->records["camera"] = projection_report(camera);
		evidence->records["effects"] = Json::Value(Json::arrayValue);
		evidence->emission_rgb.resize(static_cast<std::size_t>(width_) * height_ * 3);
	}
	if (!blur && !export_layers && !active_overlay) {
		return {std::move(sharp), std::nullopt, std::move(evidence)};
	}

	if (!static_geometry || !has_layer_geometry_) {
		layer_capture_.initialize(scene, materials, width_, height_);
		has_layer_geometry_ = true;
	}
	auto layers = layer_capture_.capture(camera, export_layers);
	if (sequence_camera == nullptr &&
	    std::none_of(layers.linear_depth.begin(), layers.linear_depth.end(),
	                 [](float depth) { return depth > 0.0f; })) {
		throw std::runtime_error("The depth layer contains no visible geometry.");
	}

	PreviewImage effects_plate;
	std::vector<float> effects_depth;
	if (active_overlay) {
		auto effects = overlay->render(scene, camera, sample_time, layers.linear_depth);
		effects_plate = effects.composite(sharp);
		evidence->records["effects"] = std::move(effects.effects);
		for (std::size_t pixel = 0; pixel < effects.emission.size(); ++pixel) {
			for (int channel = 0; channel < 3; ++channel) {
				evidence->emission_rgb[pixel * 3 + channel] = effects.emission[pixel][channel];
			}
		}
		if (blur) {
			effects_depth = layers.linear_depth;
			include_emitter_depth(effects_depth, effects.depth);
		}
	}

	const PreviewImage &plate = active_overlay ? effects_plate : sharp;
	const auto &plate_depth = active_overlay && blur ? effects_depth : layers.linear_depth;
	DepthOfFieldResult result = blur
		? composite_shot(*lens, renderer, layer_capture_, camera, settings, materials, lights,
		                 plate, plate_depth, background, active_overlay ? overlay : nullptr, scene, sample_time)
		: DepthOfFieldResult{plate, std::vector<std::uint8_t>(layers.linear_depth.size(), 0)};
	if (!export_layers) {
		return {std::move(result.color), std::nullopt, std::move(evidence)};
	}
	return {std::move(result.color),
	        RenderFrameLayers{std::move(sharp), std::move(layers), layer_capture_.labels(),
	                          std::move(result.coverage)}, std::move(evidence)};
}

std::optional<ShotCameraSample> sample_shot(const RenderJobManifest &job,
                                            const SceneGenerationContext &scene, double time_seconds)
{
	if (!job.camera) {
		return std::nullopt;
	}
	try {
		return job.camera->sample(scene, time_seconds);
	} catch (const std::exception &error) {
		throw JobFailure(ExitCode::invalid_job, error.what());
	}
}

PreviewImage render_shutter_exposure(const RenderJobManifest &job, RenderSceneTimeline &timeline,
                                     OffscreenFrameRenderer &renderer, const PreviewCameraState &fitted,
                                     double frame_time, Json::Value &metadata, Json::Value &report,
                                     const RenderExecution &execution, RenderFrameOverlay *overlay)
{
	const ShutterSettings &shutter = *job.timeline->shutter;
	const double frame_duration = 1.0 / job.timeline->frames_per_second;
	Json::Value sample_times(Json::arrayValue);
	Json::Value sample_cameras(Json::arrayValue);
	Json::Value effect_samples(Json::arrayValue);
	if (overlay) std::fill(overlay->emission_rgb.begin(), overlay->emission_rgb.end(), 0.0f);
	ShutterExposure exposure(job.width, job.height);
	for (std::uint32_t index = 0; index < shutter.samples; ++index) {
		execution.checkCancellation();
		const double time = shutter.sampleTime(frame_time, frame_duration, index);
		const auto sample = timeline.sample(time);
		const auto shot = sample_shot(job, sample.scene, time);
		auto rendered = renderer.render(sample.scene, shot, false, time, report, &fitted,
		                               job.export_layers || job.orthographic.has_value(),
		                               job.overlay ? &*job.overlay : nullptr, timeline.hasStaticGeometry());
		exposure.addSample(rendered.color);
		sample_times.append(time);
		const auto pose = shot ? frame_shot(*shot, sample.scene.getSceneBounds(), job.width, job.height) : fitted;
		sample_cameras.append(shot ? camera_report(*shot, &pose) : projection_report(pose));
		if (overlay && rendered.overlay) {
			const auto &emission = rendered.overlay->emission_rgb;
			for (std::size_t i = 0; i < emission.size(); ++i) {
				overlay->emission_rgb[i] += emission[i] / static_cast<float>(shutter.samples);
			}
			effect_samples.append(std::move(rendered.overlay->records));
		}
	}
	metadata["shutter"]["open_time_seconds"] = frame_time;
	metadata["shutter"]["close_time_seconds"] = frame_time + frame_duration * shutter.frame_fraction;
	metadata["shutter"]["sample_times_seconds"] = std::move(sample_times);
	metadata["shutter"]["sample_cameras"] = std::move(sample_cameras);
	if (overlay) overlay->records["shutter_samples"] = std::move(effect_samples);
	return exposure.finish();
}

void report_progress(const RenderExecution &execution, const RenderProgress &progress, Json::Value &report)
{
	report["rendered_frames"] = progress.rendered_frames;
	report["reused_frames"] = progress.reused_frames;
	report["completed_frames"] = progress.completedFrames();
	if (execution.publish_progress) {
		try {
			execution.publish_progress(progress);
		} catch (const std::exception &error) {
			throw JobFailure(ExitCode::output_failure, error.what());
		}
	}
}

ExitCode run_sequence(const RenderJobManifest &job, SceneGenerationRequest &request,
                      std::unique_ptr<GeneratedSceneSnapshot> initial_scene, Json::Value &report,
                      const RenderExecution &execution, RenderCheckpoint *checkpoint)
{
	const TimelineSettings &settings = *job.timeline;
	const PreviewCameraState fitted = job.orthographic ? job.orthographic->preview() :
		frame_scene(initial_scene->generationContext()->getSceneBounds(), job.width, job.height);
	try {
		std::optional<OffscreenFrameRenderer> renderer;
		if (checkpoint == nullptr) renderer.emplace(job.width, job.height, report);
		std::unique_ptr<SequenceOutput> output;
		try {
			output = std::make_unique<SequenceOutput>(job.output_directory, settings, report,
			                                          checkpoint ? checkpoint->directory() : fs::path{});
		} catch (const std::exception &error) {
			return fail(report, ExitCode::output_failure, error.what());
		}
		report["sequence_directory"] = output->directory().u8string();
		RenderProgress progress;
		report_progress(execution, progress, report);
		const auto rebuild = [&](double time_seconds) {
			request.evaluation_time = time_seconds;
			request.time_sample = true;
			auto compiled = compile_job(request);
			const std::string error = generation_error(*compiled, job.strict, report);
			if (!error.empty()) {
				report["failed_time_seconds"] = time_seconds;
				throw JobFailure(ExitCode::grammar_failure, error);
			}
			return std::move(compiled->scene_snapshot);
		};
		SequenceAudioCues audio_cues(initial_scene->grammarDocument()->audioEvents(),
		                             *initial_scene->generationContext(), settings.physics);
		RenderSceneTimeline::ObserveStep observe_step;
		if (audio_cues.observesPhysics() || execution.cancelled) {
			observe_step = [&](const SceneGenerationContext &scene, double seconds,
			                   const std::vector<SimulationContact> &contacts) {
				execution.checkCancellation();
				if (audio_cues.observesPhysics()) audio_cues.observeScene(scene, seconds, &contacts);
			};
		}
		RenderSceneTimeline timeline(std::move(initial_scene), rebuild, settings.physics,
			[&](std::uint64_t tick, const std::vector<TimelineContact> &contacts) {
				try {
					output->recordContacts(tick, contacts);
				} catch (const std::exception &error) {
					throw JobFailure(ExitCode::output_failure, error.what());
				}
			}, std::move(observe_step));

		const auto first_observed_frame = audio_cues.observesFrames() ? 0u : settings.start_frame;
		const bool observe_reused_pose = settings.physics || audio_cues.observesFrames();
		const auto reuse_frame = [&](std::uint32_t number, const Json::Value &saved) {
			output->reuseFrame(number, job.overlay.has_value());
			report["pixels_different_from_background"] = saved["pixels_different_from_background"];
			++progress.reused_frames;
			report_progress(execution, progress, report);
			report.removeMember("failed_frame");
		};
		for (std::uint32_t frame_number = first_observed_frame; frame_number < settings.end_frame; ++frame_number) {
			execution.checkCancellation();
			report["failed_frame"] = frame_number;
			std::optional<Json::Value> saved;
			if (checkpoint && frame_number >= settings.start_frame) {
				try {
					saved = checkpoint->restoreFrame(frame_number, job.width, job.height, job.export_layers,
					                                 job.overlay.has_value());
				} catch (const std::exception &error) {
					throw JobFailure(ExitCode::output_failure, error.what());
				}
			}
			if (saved && !observe_reused_pose) {
				reuse_frame(frame_number, *saved);
				continue;
			}

			const double time_seconds = settings.frameTime(frame_number);
			const auto sample = timeline.sample(time_seconds);
			const auto shot = sample_shot(job, sample.scene, time_seconds);
			audio_cues.observeFrame(sample.scene, time_seconds);
			if (audio_cues.observesCamera()) {
				const auto position = shot ? shot->position : preview_camera_matrices(fitted).position / fitted.scale_global;
				audio_cues.observeCamera(position, time_seconds);
			}
			if (frame_number < settings.start_frame) continue;

			if (saved) {
				if (settings.physics && settings.shutter && settings.shutter->frame_fraction > 0.0) {
					const auto &shutter = *settings.shutter;
					timeline.sample(shutter.sampleTime(time_seconds, 1.0 / settings.frames_per_second, shutter.samples - 1));
				}
				reuse_frame(frame_number, *saved);
				continue;
			}

			Json::Value metadata;
			metadata["frame"] = frame_number;
			metadata["time_seconds"] = time_seconds;
			metadata["simulation_tick"] = static_cast<Json::UInt64>(sample.simulation_tick);
			metadata["simulation_fraction"] = sample.simulation_fraction;
			if (settings.export_transforms) metadata["transforms"] = sample_transforms(sample);

			if (shot) {
				const auto pose = frame_shot(*shot, sample.scene.getSceneBounds(), job.width, job.height);
				metadata["camera"] = camera_report(*shot, &pose);
			} else if (job.orthographic) {
				metadata["camera"] = job.orthographic->metadata();
			}
			if (!renderer) renderer.emplace(job.width, job.height, report);
			auto frame = renderer->render(sample.scene, shot, job.export_layers, time_seconds, report, &fitted,
			                              job.orthographic.has_value(), job.overlay ? &*job.overlay : nullptr,
			                              timeline.hasStaticGeometry());
			metadata["pixels_different_from_background"] = report["pixels_different_from_background"];
			if (settings.shutter && settings.shutter->frame_fraction > 0.0) {
				frame.color = render_shutter_exposure(job, timeline, *renderer, fitted, time_seconds, metadata, report,
				                                      execution, frame.overlay ? &*frame.overlay : nullptr);
				report["pixels_different_from_background"] = metadata["pixels_different_from_background"];
			}
			try {
				output->publishFrame(frame_number, frame, std::move(metadata));
			} catch (const std::exception &error) {
				throw JobFailure(ExitCode::output_failure, error.what());
			}
			++progress.rendered_frames;
			report_progress(execution, progress, report);
			report.removeMember("failed_frame");
		}
		const double end_seconds = settings.frameTime(settings.end_frame);
		const auto last_cue_tick = (static_cast<std::uint64_t>(settings.end_frame) *
		                            kSceneSimulationTicksPerSecond - 1) / settings.frames_per_second;
		if (audio_cues.observesPhysics() && last_cue_tick > timeline.solvedTick()) {
			timeline.sample(static_cast<double>(last_cue_tick) / kSceneSimulationTicksPerSecond);
		}
		execution.checkCancellation();
		try {
			report["sequence_manifest"] = output->finish(timeline.solvedTick(),
			                                             audio_cues.manifest(end_seconds)).u8string();
			report["contacts"] = (output->directory() / "contacts.jsonl").u8string();
		} catch (const std::exception &error) {
			return fail(report, ExitCode::output_failure, error.what());
		}
	} catch (const RenderCancelled &error) {
		report.removeMember("failed_frame");
		return fail(report, ExitCode::cancelled, error.what());
	} catch (const JobFailure &error) {
		return fail(report, error.code(), error.what());
	} catch (const std::exception &error) {
		return fail(report, ExitCode::render_failure, error.what());
	}
	return ExitCode::success;
}

void report_still_checkpoint(const fs::path &directory, const Json::Value &frame, Json::Value &report)
{
	const auto &files = frame["files"];
	report["image"] = (directory / fs::u8path(files["preview"].asString())).u8string();
	report["frame_manifest"] = (directory / "frame.json").u8string();
	report["checksums"] = (directory / "checksums.json").u8string();
	report["pixels_different_from_background"] = frame["pixels_different_from_background"];
	report["pixel_hash_fnv1a64"] = frame["pixel_hash_fnv1a64"];
	if (files.isMember("color") || files.isMember("emission")) {
		for (const auto &key : files.getMemberNames()) {
			report["layers"][key] = (directory / fs::u8path(files[key].asString())).u8string();
		}
		report["layers"]["manifest"] = report["frame_manifest"];
	}
}

std::set<double> cinematic_validation_times(const RenderJobManifest &job)
{
	const auto &timeline = *job.timeline;
	const double start = timeline.frameTime(timeline.start_frame);
	const double last_frame = timeline.frameTime(timeline.end_frame - 1);
	const double last_sample = timeline.shutter && timeline.shutter->frame_fraction > 0.0
		? timeline.shutter->sampleTime(last_frame, 1.0 / timeline.frames_per_second, timeline.shutter->samples - 1)
		: last_frame;
	std::set<double> times{start, (start + last_sample) * 0.5, last_frame, last_sample};
	const auto add_keys = [&](const Json::Value &keys) {
		if (!keys.isArray()) return;
		for (const auto &key : keys) {
			const double time = key["time"].asDouble();
			if (time >= start && time <= last_sample) times.insert(time);
		}
	};
	const auto &manifest = job.snapshot();
	if (job.camera) {
		const auto &camera = manifest["camera"];
		add_keys(camera["aim"]);
		add_keys(camera["focus"]);
		add_keys(camera["roll"]);
		if (camera.isMember("rail")) add_keys(camera["rail"]["progress"]);
	}
	if (job.overlay) {
		for (const auto &effect : manifest["overlays"]["effects"]) {
			if (effect.isMember("active")) {
				const double on = std::max(start, effect["active"][0].asDouble());
				const double off = std::min(last_sample, effect["active"][1].asDouble());
				if (on <= off) {
					times.insert(on);
					times.insert((on + off) * 0.5);
				}
			}
			add_keys(effect["gate"]);
		}
	}
	return times;
}

void validate_cinematic_timeline(const RenderJobManifest &job, SceneGenerationRequest request,
                                 std::unique_ptr<GeneratedSceneSnapshot> initial, Json::Value &report)
{
	RenderSceneTimeline timeline(std::move(initial), [&](double time) {
		request.evaluation_time = time;
		request.time_sample = true;
		auto compiled = compile_job(request);
		Json::Value diagnostics;
		const auto error = generation_error(*compiled, job.strict, diagnostics);
		if (!error.empty()) throw JobFailure(ExitCode::grammar_failure, error);
		return std::move(compiled->scene_snapshot);
	}, job.timeline->physics, {});
	Json::Value sampled(Json::arrayValue);
	for (const double time : cinematic_validation_times(job)) {
		report["failed_time_seconds"] = time;
		const auto sample = timeline.sample(time);
		if (job.overlay) job.overlay->validate(sample.scene, time);
		Json::Value key;
		key["time_seconds"] = time;
		if (const auto shot = sample_shot(job, sample.scene, time)) {
			const auto pose = frame_shot(*shot, sample.scene.getSceneBounds(), job.width, job.height);
			if (sampled.empty()) report["camera"] = camera_report(*shot, &pose);
			if (shot->rail) {
				key["distance"] = shot->rail->distance;
				key["speed"] = shot->rail->speed;
			}
		}
		sampled.append(std::move(key));
	}
	report.removeMember("failed_time_seconds");
	report["cinematic_samples"] = std::move(sampled);
}

ExitCode run_job(const RenderJobManifest &job, bool render, Json::Value &report,
                 const RenderExecution &execution = {})
{
	report["job"] = job.manifest.u8string();
	report["grammar"] = job.grammar.u8string();
	report["seed"] = static_cast<Json::UInt64>(job.seed);
	report["strict"] = job.strict;
	report["width"] = job.width;
	report["height"] = job.height;
	report["warnings"] = Json::Value(Json::arrayValue);
	report["runtime_errors"] = Json::Value(Json::arrayValue);
	if (job.timeline) {
		report["timeline"] = timeline_report(*job.timeline);
	}
	if (job.orthographic) report["camera"] = job.orthographic->metadata();

	SceneGenerationRequest request;
	std::unique_ptr<SceneGenerationResult> compiled;
	try {
		execution.checkCancellation();
		request = scene_request(job, execution.grammar_snapshot);
		compiled = compile_job(request);
		execution.checkCancellation();
	} catch (const RenderCancelled &error) {
		return fail(report, ExitCode::cancelled, error.what());
	} catch (const std::exception &error) {
		return fail(report, ExitCode::invalid_job, error.what());
	}

	const std::string error = generation_error(*compiled, job.strict, report);
	if (!error.empty()) {
		return fail(report, ExitCode::grammar_failure, error);
	}

	const SceneGenerationContext &scene = *compiled->scene_snapshot->generationContext();
	report["primitive_count"] = static_cast<Json::UInt64>(std::count_if(
		scene.primitive_instances.begin(), scene.primitive_instances.end(),
		[](const PrimitiveInstance &instance) { return !instance.removed; }));
	std::unique_ptr<RenderCheckpoint> checkpoint;
	if (render && !execution.checkpoint_directory.empty()) {
		try {
			checkpoint = std::make_unique<RenderCheckpoint>(execution.checkpoint_directory, execution.resume,
			                                               job.snapshot(), request.source_text);
		} catch (const std::exception &error) {
			return fail(report, ExitCode::output_failure, error.what());
		}
	}
	if (job.timeline && render) {
		return run_sequence(job, request, std::move(compiled->scene_snapshot), report, execution, checkpoint.get());
	}
	if (!render && job.timeline && (job.overlay || (job.camera && job.snapshot()["camera"].isMember("rail")))) {
		try {
			validate_cinematic_timeline(job, request, std::move(compiled->scene_snapshot), report);
			return ExitCode::success;
		} catch (const JobFailure &error) {
			return fail(report, error.code(), error.what());
		} catch (const std::exception &error) {
			return fail(report, ExitCode::invalid_job, error.what());
		}
	}

	std::optional<ShotCameraSample> shot;
	if (job.camera) {
		try {
			shot = job.timeline ? job.camera->sample(scene, 0.0) : job.camera->sample(scene);
			const auto pose = frame_shot(*shot, scene.getSceneBounds(), job.width, job.height);
			report["camera"] = camera_report(*shot, &pose);
		} catch (const std::exception &error) {
			return fail(report, ExitCode::invalid_job, error.what());
		}
	}
	if (job.overlay) {
		try {
			job.overlay->validate(scene, shot ? shot->time_seconds : 0.0);
		} catch (const std::exception &error) {
			return fail(report, ExitCode::invalid_job, error.what());
		}
	}
	if (!render) {
		return ExitCode::success;
	}

	if (checkpoint) {
		try {
			execution.checkCancellation();
			if (const auto saved = checkpoint->restoreFrame(0, job.width, job.height, job.export_layers, job.overlay.has_value())) {
				report_still_checkpoint(sequence_frame_directory(checkpoint->directory(), 0), *saved, report);
				report_progress(execution, {0, 1}, report);
				return ExitCode::success;
			}
		} catch (const RenderCancelled &error) {
			return fail(report, ExitCode::cancelled, error.what());
		} catch (const std::exception &error) {
			return fail(report, ExitCode::output_failure, error.what());
		}
	}

	RenderedFrame frame;
	try {
		execution.checkCancellation();
		OffscreenFrameRenderer renderer(job.width, job.height, report);
		const auto calibrated = job.orthographic ? std::optional<PreviewCameraState>(job.orthographic->preview()) : std::nullopt;
		frame = renderer.render(scene, shot, job.export_layers, shot ? shot->time_seconds : 0.0, report,
		                        calibrated ? &*calibrated : nullptr,
		                        calibrated.has_value(), job.overlay ? &*job.overlay : nullptr);
		if (calibrated && report["pixels_different_from_background"].asUInt64() == 0) {
			throw std::runtime_error("The calibrated camera sees no pixels from the model.");
		}
		execution.checkCancellation();
	} catch (const RenderCancelled &error) {
		return fail(report, ExitCode::cancelled, error.what());
	} catch (const std::exception &error) {
		return fail(report, ExitCode::render_failure, error.what());
	}

	try {
		if (checkpoint) {
			Json::Value metadata;
			metadata["frame"] = 0;
			for (const char *key : {"seed", "grammar", "pixels_different_from_background"}) metadata[key] = report[key];
			if (report.isMember("camera")) metadata["camera"] = report["camera"];
			const auto manifest = publish_sequence_frame(checkpoint->directory(), 0, frame, std::move(metadata));
			report_still_checkpoint(manifest.parent_path(), manifest_input::read_json(manifest), report);
			report_progress(execution, {1, 0}, report);
			return ExitCode::success;
		}
		if (frame.layers || frame.overlay) {
			publish_layered_frame(job.output_directory, frame, report);
			return ExitCode::success;
		}
		const fs::path output = job.output_directory / "preview.ppm";
		if (fs::exists(output) && (fs::equivalent(output, job.grammar) || fs::equivalent(output, job.manifest))) {
			throw std::runtime_error("preview.ppm cannot replace the grammar or job input file.");
		}
		fs::create_directories(job.output_directory);
		render_files::AtomicFile pending(output);
		const auto evidence = PreviewFramebufferCaptureService().writePreview(frame.color, pending.staging());
		if (!evidence.succeeded()) {
			return fail(report, ExitCode::output_failure, evidence.message());
		}
		pending.publish();
		report["image"] = output.u8string();
		report["pixel_hash_fnv1a64"] = std::to_string(evidence.pixelHash());
	} catch (const std::exception &error) {
		return fail(report, ExitCode::output_failure, error.what());
	}
	return ExitCode::success;
}

void probe_graphics(Json::Value &report)
{
	OffscreenRenderContext context;
	context.initialize(report);
	PreviewCameraState camera;
	camera.width = 16;
	camera.height = 16;
	preview_renderer().render(camera, PreviewRenderSettings{}, {}, PreviewLightCollection{});
	read_preview(camera);
}

std::unique_ptr<GeneratedSceneSnapshot> compile_experiment_scene(const fs::path &grammar,
                                                                const std::string &text, std::uint64_t seed,
                                                                Json::Value &report)
{
	SceneGenerationRequest request;
	request.source_name = grammar.u8string();
	request.source_text = text;
	request.design_nonce = seed;
	report["warnings"] = Json::Value(Json::arrayValue);
	report["runtime_errors"] = Json::Value(Json::arrayValue);
	auto compiled = compile_job(request);
	const std::string error = generation_error(*compiled, true, report);
	if (!error.empty()) {
		report["error"] = error;
		return nullptr;
	}
	return std::move(compiled->scene_snapshot);
}

ExitCode run_sweep(const fs::path &manifest, const fs::path &destination, Json::Value &report)
{
	const RenderSweep sweep(manifest);
	const RenderJobManifest job(sweep.job());
	if (job.timeline) throw std::runtime_error("Sweep uses a still render job.");
	if (sweep.variesAperture() && !job.camera) throw std::runtime_error("Sweeping aperture requires a shot camera and focus track.");
	Json::Value baseline_report;
	const auto baseline = compile_experiment_scene(job.grammar, render_files::read_text(job.grammar), job.seed, baseline_report);
	if (!baseline) {
		report["baseline"] = baseline_report;
		return fail(report, ExitCode::grammar_failure, baseline_report["error"].asString());
	}
	const auto &scene = *baseline->generationContext();
	const auto bounds = scene.getSceneBounds();
	if (!bounds.valid) return fail(report, ExitCode::render_failure, "The sweep baseline has no drawable geometry.");
	const auto fitted = job.orthographic ? job.orthographic->preview() : frame_scene(bounds, job.width, job.height);
	const auto baseline_shot = job.camera ? std::optional<ShotCameraSample>(job.camera->sample(scene)) : std::nullopt;
	Json::Value capture;
	if (baseline_shot) capture["camera"] = camera_report(*baseline_shot);
	else if (job.orthographic) capture["camera"] = job.orthographic->metadata();
	else capture["camera"]["projection"] = "auto_fitted_to_baseline";
	capture["seed"] = static_cast<Json::UInt64>(job.seed);
	capture["width"] = job.width;
	capture["height"] = job.height;
	std::optional<OffscreenFrameRenderer> renderer;
	const bool succeeded = sweep.run(job.grammar, destination, capture,
		[&](const std::string &text, std::optional<float> aperture, Json::Value &variant) -> PreviewImage {
			const auto snapshot = compile_experiment_scene(job.grammar, text, job.seed, variant);
			if (!snapshot) return {};
			auto shot = baseline_shot;
			if (aperture) shot->lens.aperture_radius = *aperture;
			if (!renderer) renderer.emplace(job.width, job.height, report);
			auto frame = renderer->render(*snapshot->generationContext(), shot, sweep.object().has_value(),
			                              0.0, variant, &fitted, true);
			if (variant["pixels_different_from_background"].asUInt64() == 0) {
				throw std::runtime_error("No model pixels differ from the background.");
			}
			if (sweep.object()) {
				const auto &layers = *frame.layers;
				const auto label = std::find_if(layers.labels.begin(), layers.labels.end(), [&](const auto &entry) {
					return entry.object == *sweep.object();
				});
				const auto count = label == layers.labels.end() ? 0 :
					std::count(layers.geometry.object_ids.begin(), layers.geometry.object_ids.end(),
					           static_cast<std::uint32_t>(label - layers.labels.begin()));
				variant["visible_object_pixels"] = static_cast<Json::UInt64>(count);
				if (count == 0) throw std::runtime_error("No visible pixels for object " + *sweep.object() + ".");
			}
			return std::move(frame.color);
		}, report["sweep"]);
	return succeeded ? ExitCode::success : fail(report, ExitCode::render_failure, "One or more sweep variants failed. See metrics.json.");
}

ExitCode run_fit(const fs::path &grammar, const fs::path &views, const fs::path &destination, Json::Value &report)
{
	std::optional<OffscreenFrameRenderer> renderer;
	int width = 0, height = 0;
	const bool succeeded = fit_grammar_images(grammar, views, destination, compile_experiment_scene,
		[&](const SceneGenerationContext &scene, const CalibratedCamera &camera) {
			const auto view = camera.preview();
			if (!renderer || width != view.width || height != view.height) {
				renderer.reset();
				width = view.width;
				height = view.height;
				renderer.emplace(width, height, report);
			}
			Json::Value evidence;
			return renderer->render(scene, std::nullopt, false, 0.0, evidence, &view, true).color;
		}, report["fit"]);
	return succeeded ? ExitCode::success : fail(report, ExitCode::render_failure, report["fit"]["error"].asString());
}

Json::Value run_queued_render(const fs::path &manifest, const RenderExecution &execution)
{
	Json::Value report;
	report["schema_version"] = schema_version;
	report["command"] = "render";
	ExitCode result = ExitCode::invalid_job;
	try {
		result = run_job(RenderJobManifest(manifest), true, report, execution);
	} catch (const std::exception &error) {
		result = fail(report, result, error.what());
	}
	report["ok"] = result == ExitCode::success;
	report["exit_code"] = static_cast<int>(result);
	return report;
}

ExitCode run_queue(std::wstring_view command, int argc, wchar_t **argv, Json::Value &report)
{
	int index = 2;
	std::wstring_view target;
	if (index < argc && std::wstring_view(argv[index]).substr(0, 2) != L"--") target = argv[index++];
	fs::path directory;
	bool once = false;
	while (index < argc) {
		const std::wstring_view option = argv[index++];
		if (option == L"--queue" && directory.empty() && index < argc && argv[index][0] != L'\0' &&
		    std::wstring_view(argv[index]).substr(0, 2) != L"--") {
			directory = argv[index++];
		} else if (option == L"--once" && command == L"worker" && !once) {
			once = true;
		} else {
			throw std::runtime_error("Invalid render queue arguments.");
		}
	}
	if ((command == L"worker" && !target.empty()) ||
	    ((command == L"enqueue" || command == L"resume" || command == L"cancel") && target.empty())) {
		throw std::runtime_error("Invalid render queue arguments.");
	}
	if (directory.empty()) directory = progen3d_path_locator().statePath("render-queue");
	const RenderQueue queue(directory);
	report["command"] = std::string(command.begin(), command.end());
	report["queue_directory"] = queue.directory().u8string();
	if (command == L"worker") {
		try {
			queue.work(once, [](Json::Value &result) {
				try {
					probe_graphics(result);
				} catch (const std::exception &error) {
					throw JobFailure(ExitCode::render_failure, error.what());
				}
			}, run_queued_render, report);
		} catch (const JobFailure &error) {
			return fail(report, error.code(), error.what());
		}
	} else if (command == L"enqueue") {
		const RenderJobManifest job{fs::path(target)};
		report["job"] = queue.enqueue(job.manifest, job.snapshot(), render_files::read_text(job.grammar), job.totalFrames());
	} else {
		if (!std::all_of(target.begin(), target.end(), [](wchar_t ch) { return ch >= L'0' && ch <= L'9'; })) {
			throw std::runtime_error("Expected a 12-digit render job ID.");
		}
		const std::string id(target.begin(), target.end());
		if (command == L"status") {
			const auto status = queue.status(id.empty() ? std::nullopt : std::optional<std::string>{id});
			for (const auto &key : status.getMemberNames()) report[key] = status[key];
		} else {
			report["job"] = command == L"cancel" ? queue.cancel(id) : queue.resume(id);
		}
	}
	return ExitCode::success;
}

ExitCode run(int argc, wchar_t **argv, Json::Value &report)
{
	const std::wstring_view command = argc >= 2 ? argv[1] : L"";
	if (command == L"--help" && argc == 2) {
		report["usage"] = usage;
		return ExitCode::success;
	}
	if (command == L"doctor" && argc == 2) {
		report["command"] = "doctor";
		try {
			probe_graphics(report);
		} catch (const std::exception &error) {
			return fail(report, ExitCode::render_failure, error.what());
		}
		return ExitCode::success;
	}
	if (command == L"enqueue" || command == L"status" || command == L"cancel" || command == L"resume" || command == L"worker") {
		return run_queue(command, argc, argv, report);
	}
	if (command == L"compose" && argc == 5 && std::wstring_view(argv[3]) == L"--out") {
		report["command"] = "compose";
		const VideoEdit edit{fs::path(argv[2])};
		try {
			report["composition"] = compose_video(edit, fs::path(argv[4]));
		} catch (const std::exception &error) {
			return fail(report, ExitCode::output_failure, error.what());
		}
		return ExitCode::success;
	}
	if (command == L"sweep" && argc == 5 && std::wstring_view(argv[3]) == L"--out" && argv[4][0] != L'\0') {
		report["command"] = "sweep";
		return run_sweep(fs::path(argv[2]), fs::path(argv[4]), report);
	}
	if (command == L"fit" && argc == 7 && std::wstring_view(argv[3]) == L"--views" &&
	    std::wstring_view(argv[5]) == L"--out" && argv[6][0] != L'\0') {
		report["command"] = "fit";
		return run_fit(fs::path(argv[2]), fs::path(argv[4]), fs::path(argv[6]), report);
	}
	if (command == L"validate" && argc == 3) {
		report["command"] = "validate";
		return run_job(RenderJobManifest(fs::path(argv[2])), false, report);
	}
	if (command == L"render" && argc >= 3) {
		report["command"] = "render";
		RenderExecution execution;
		if ((argc == 5 || argc == 6) && std::wstring_view(argv[3]) == L"--out" && argv[4][0] != L'\0' &&
		    (argc == 5 || std::wstring_view(argv[5]) == L"--resume")) {
			execution.checkpoint_directory = argv[4];
			execution.resume = argc == 6;
		} else if (argc != 3) {
			return fail(report, ExitCode::invalid_job, "Expected render <job.json> [--out <directory> [--resume]].");
		}
		return run_job(RenderJobManifest(fs::path(argv[2])), true, report, execution);
	}
	report["usage"] = usage;
	return fail(report, ExitCode::invalid_job, "Expected a command and its required arguments.");
}

}

void debugout(const std::string &)
{
}

void errorout(std::string message)
{
	engine_errors.push_back(std::move(message));
}

int wmain(int argc, wchar_t **argv)
{
	progen3d_initialize_paths(nullptr);
	Json::Value report(Json::objectValue);
	report["schema_version"] = schema_version;
	ExitCode result = ExitCode::invalid_job;
	try {
		result = run(argc, argv, report);
	} catch (const std::exception &error) {
		result = fail(report, ExitCode::invalid_job, error.what());
	}
	report["ok"] = result == ExitCode::success;
	report["exit_code"] = static_cast<int>(result);
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "";
	std::cout << Json::writeString(writer, report) << '\n';
	return static_cast<int>(result);
}
