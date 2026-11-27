#include "GrammarImageFit.h"

#include "Context.h"
#include "ExperimentImages.h"
#include "ExperimentParameters.h"
#include "ManifestInput.h"
#include "RenderFiles.h"
#include "ShotCamera.h"
#include "geometry/service/BinarySilhouetteComparisonService.h"
#include "geometry/service/ThreeViewProjectionService.h"
#include "imgui_render.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>

namespace {

namespace fs = std::filesystem;

struct LandmarkObservation {
	ShotAttachment target;
	glm::vec2 pixel;
};

struct ReferenceView {
	std::string id;
	CalibratedCamera camera;
	BinarySilhouette silhouette;
	std::optional<PreviewImage> image;
	std::vector<LandmarkObservation> landmarks;
	bool held_out;
};

struct FitSpecification {
	std::uint64_t seed;
	ParameterGrid geometry;
	ParameterGrid appearance;
	std::vector<ReferenceView> views;
	double landmark_weight;
};

BinarySilhouette binary_mask(const PreviewImage &image)
{
	BinarySilhouette mask(image.width(), image.height());
	for (int row = 0; row < image.height(); ++row) {
		for (int column = 0; column < image.width(); ++column) {
			const auto *rgb = image.rgbPixels().data() + (static_cast<std::size_t>(row) * image.width() + column) * 3;
			if (rgb[0] || rgb[1] || rgb[2]) mask.occupy(column, row);
		}
	}
	return mask;
}

std::vector<LandmarkObservation> read_landmarks(const Json::Value &list, int width, int height)
{
	std::vector<LandmarkObservation> landmarks;
	if (list.isNull()) return landmarks;
	if (!list.isArray()) throw std::runtime_error("View landmarks must be an array.");
	for (const auto &entry : list) {
		manifest_input::check_fields(entry, {"object", "attachment", "pixel"}, "view landmark");
		const auto &pixel = entry["pixel"];
		if (!pixel.isArray() || pixel.size() != 2) throw std::runtime_error("Landmark pixel needs two coordinates.");
		const auto x = manifest_input::read_number(pixel[0], "landmark x");
		const auto y = manifest_input::read_number(pixel[1], "landmark y");
		if (x < 0 || x > width || y < 0 || y > height) throw std::runtime_error("Landmark lies outside its reference image.");
		ShotAttachment target;
		target.object = manifest_input::read_string(entry["object"], "landmark.object");
		if (entry.isMember("attachment")) {
			target.attachment = manifest_input::read_string(entry["attachment"], "landmark.attachment");
		}
		landmarks.push_back({std::move(target), {x, y}});
	}
	return landmarks;
}

ReferenceView read_view(const Json::Value &entry, const fs::path &directory)
{
	manifest_input::check_fields(entry, {"id", "camera", "silhouette", "image", "landmarks", "held_out"}, "reference view");
	const auto id = manifest_input::read_string(entry["id"], "view.id");
	if (!experiment_identifier(id)) throw std::runtime_error("View IDs must be identifiers.");
	const auto mask_image = read_experiment_image(manifest_input::resolve_path(directory, entry["silhouette"], "view.silhouette"));
	auto mask = binary_mask(mask_image);
	if (mask.occupiedPixelCount() == 0) throw std::runtime_error("Reference silhouette is empty: " + id);
	const auto camera = CalibratedCamera::parse(entry["camera"], mask_image.width(), mask_image.height());
	std::optional<PreviewImage> image;
	if (entry.isMember("image")) {
		image = read_experiment_image(manifest_input::resolve_path(directory, entry["image"], "view.image"));
		if (image->width() != mask_image.width() || image->height() != mask_image.height()) {
			throw std::runtime_error("Reference image and silhouette dimensions differ: " + id);
		}
	}
	if (entry.isMember("held_out") && !entry["held_out"].isBool()) throw std::runtime_error("held_out must be a boolean.");
	return {id, camera, std::move(mask), std::move(image),
	        read_landmarks(entry["landmarks"], mask_image.width(), mask_image.height()), entry["held_out"].asBool()};
}

FitSpecification read_specification(const fs::path &path)
{
	const auto root = manifest_input::read_json(path);
	manifest_input::check_fields(root, {"schema_version", "seed", "geometry", "appearance", "views", "landmark_weight"}, "fit manifest");
	if (root["schema_version"] != 1 || !manifest_input::is_unsigned_integer(root["seed"])) {
		throw std::runtime_error("Fit requires schema_version 1 and a seed.");
	}
	if (!root["views"].isArray() || root["views"].empty()) throw std::runtime_error("Fit requires calibrated reference views.");
	const auto weight = root.isMember("landmark_weight") ? manifest_input::read_number(root["landmark_weight"], "landmark_weight") : 1.0;
	if (weight < 0) throw std::runtime_error("landmark_weight must be nonnegative.");
	FitSpecification spec{root["seed"].asUInt64(), ParameterGrid::parse(root["geometry"], "geometry"),
	                      ParameterGrid::parse(root["appearance"], "appearance"), {}, weight};
	std::set<std::string> names;
	for (const auto &axis : spec.geometry.axes()) {
		names.insert(axis.name);
		if (!std::all_of(axis.values.begin(), axis.values.end(), [](const auto &value) { return value.isNumeric(); })) {
			throw std::runtime_error("Geometry axes must contain numeric values: " + axis.name);
		}
	}
	for (const auto &axis : spec.appearance.axes()) {
		if (!names.insert(axis.name).second) throw std::runtime_error("A parameter cannot be in both fitting phases: " + axis.name);
	}
	if (names.empty()) throw std::runtime_error("Fit requires geometry or appearance parameters.");
	std::set<std::string> views;
	for (const auto &entry : root["views"]) {
		auto view = read_view(entry, path.parent_path());
		if (!views.insert(view.id).second) throw std::runtime_error("Duplicate view ID: " + view.id);
		spec.views.push_back(std::move(view));
	}
	if (std::none_of(spec.views.begin(), spec.views.end(), [](const auto &view) { return !view.held_out; })) {
		throw std::runtime_error("Fit needs at least one training view.");
	}
	if (!spec.appearance.axes().empty() && std::none_of(spec.views.begin(), spec.views.end(), [](const auto &view) {
		return !view.held_out && view.image.has_value();
	})) {
		throw std::runtime_error("Appearance fitting needs an RGB image in at least one training view.");
	}
	return spec;
}

struct ViewMeasurement {
	BinarySilhouette predicted;
	Json::Value scores;
	double geometry_loss;
};

Json::Value pixel_json(const glm::vec2 &point)
{
	Json::Value pixel(Json::arrayValue);
	pixel.append(point.x);
	pixel.append(point.y);
	return pixel;
}

ViewMeasurement measure_view(const ReferenceView &view, const SceneGenerationContext &scene,
                             const Mesh &mesh, double landmark_weight)
{
	auto predicted = ThreeViewProjectionService().projectCalibrated(mesh, view.camera.projection());
	const auto comparison = BinarySilhouetteComparisonService().compare(predicted, view.silhouette);
	Json::Value scores;
	scores["id"] = view.id;
	scores["role"] = view.held_out ? "held_out" : "training";
	scores["silhouette_iou"] = comparison.intersectionOverUnion();
	scores["intersection_pixels"] = static_cast<Json::UInt64>(comparison.intersectionPixels());
	scores["union_pixels"] = static_cast<Json::UInt64>(comparison.unionPixels());
	const auto predicted_pixels = predicted.occupiedPixelCount();
	scores["predicted_pixels"] = static_cast<Json::UInt64>(predicted_pixels);

	double loss = 1 - comparison.intersectionOverUnion();
	if (!view.landmarks.empty()) {
		double sum_squared = 0;
		for (const auto &landmark : view.landmarks) {
			const auto point = view.camera.projection().imagePoint(resolve_scene_attachment(scene, landmark.target));
			if (!std::isfinite(point.x) || !std::isfinite(point.y)) throw std::runtime_error("Nonfinite projected landmark.");
			const glm::dvec2 error = glm::dvec2(point) - glm::dvec2(landmark.pixel);
			const double squared = glm::dot(error, error);
			sum_squared += squared;
			Json::Value entry;
			entry["object"] = landmark.target.object;
			if (!landmark.target.attachment.empty()) entry["attachment"] = landmark.target.attachment;
			entry["reference_pixel"] = pixel_json(landmark.pixel);
			entry["fitted_pixel"] = pixel_json(point);
			entry["error_pixels"] = std::sqrt(squared);
			scores["landmarks"].append(std::move(entry));
		}
		const double rms = std::sqrt(sum_squared / view.landmarks.size());
		scores["landmark_rmse_pixels"] = rms;
		loss += landmark_weight * rms / std::hypot(predicted.width(), predicted.height());
	}
	return {std::move(predicted), std::move(scores), loss};
}

double color_rmse(const PreviewImage &candidate, const ReferenceView &view, const BinarySilhouette &predicted)
{
	const auto &reference = *view.image;
	if (!candidate.isValid() || candidate.width() != reference.width() || candidate.height() != reference.height()) {
		throw std::runtime_error("Calibrated render dimensions differ from reference: " + view.id);
	}
	double sum = 0;
	std::size_t channels = 0;
	const auto &candidate_rgb = candidate.rgbPixels();
	const auto &reference_rgb = reference.rgbPixels();
	for (std::size_t pixel = 0; pixel < predicted.pixels().size(); ++pixel) {
		if (!predicted.pixels()[pixel] || !view.silhouette.pixels()[pixel]) continue;
		for (std::size_t channel = pixel * 3; channel < pixel * 3 + 3; ++channel) {
			const double difference = static_cast<double>(candidate_rgb[channel]) - reference_rgb[channel];
			sum += difference * difference;
		}
		channels += 3;
	}
	if (channels == 0) throw std::runtime_error("No overlapping pixels for color comparison: " + view.id);
	return std::sqrt(sum / channels);
}

PreviewImage difference_overlay(const ReferenceView &view, const ViewMeasurement &measurement)
{
	const auto &candidate = measurement.predicted;
	const auto &reference = view.silhouette;
	constexpr std::array<std::array<std::uint8_t, 3>, 4> colors{{
		{17, 27, 36}, {53, 200, 225}, {250, 123, 90}, {211, 225, 232}
	}};
	std::vector<std::uint8_t> pixels(candidate.pixels().size() * 3);
	for (std::size_t pixel = 0; pixel < candidate.pixels().size(); ++pixel) {
		const auto &color = colors[candidate.pixels()[pixel] + 2 * reference.pixels()[pixel]];
		std::copy_n(color.begin(), 3, pixels.begin() + pixel * 3);
	}
	return {static_cast<int>(candidate.width()), static_cast<int>(candidate.height()), std::move(pixels)};
}

struct FitCandidate {
	Json::Value parameters;
	std::unique_ptr<GeneratedSceneSnapshot> scene;
	double loss;
};

std::optional<FitCandidate> fit_geometry(const FitSpecification &specification, const ParameterizedGrammar &grammar,
                                         const fs::path &source, const CompileExperimentScene &compile,
                                         Json::Value &search)
{
	std::optional<FitCandidate> best;
	specification.geometry.forEach([&](const Json::Value &parameters) {
		Json::Value row;
		row["parameters"] = parameters;
		try {
			auto snapshot = compile(source, grammar.apply(parameters), specification.seed, row);
			if (snapshot) {
				const auto &scene = *snapshot->generationContext();
				const auto cube = build_base_vertex_data();
				const auto mesh = scene.buildExportMesh(cube.data());
				double loss = 0;
				std::size_t training_views = 0;
				for (const auto &view : specification.views) {
					if (view.held_out) continue;
					const auto measurement = measure_view(view, scene, mesh, specification.landmark_weight);
					if (measurement.scores["predicted_pixels"].asUInt64() == 0) {
						throw std::runtime_error("The candidate has an empty training silhouette: " + view.id);
					}
					loss += measurement.geometry_loss;
					++training_views;
				}
				loss /= training_views;
				row["loss"] = loss;
				if (!best || loss < best->loss) best = FitCandidate{parameters, std::move(snapshot), loss};
			}
		} catch (const std::runtime_error &error) {
			row["error"] = error.what();
		}
		row["ok"] = !row.isMember("error");
		search.append(std::move(row));
	});
	return best;
}

std::optional<FitCandidate> fit_appearance(const FitSpecification &specification, const ParameterizedGrammar &grammar,
                                           const fs::path &source, const FitCandidate &geometry,
                                           const RenderFitView &render, const CompileExperimentScene &compile,
                                           Json::Value &search)
{
	const auto cube = build_base_vertex_data();
	const auto fixed_mesh = geometry.scene->generationContext()->buildExportMesh(cube.data());
	std::vector<std::pair<const ReferenceView *, BinarySilhouette>> training;
	for (const auto &view : specification.views) {
		if (!view.held_out && view.image) {
			training.emplace_back(&view, ThreeViewProjectionService().projectCalibrated(fixed_mesh, view.camera.projection()));
		}
	}
	std::optional<FitCandidate> best;
	specification.appearance.forEach([&](const Json::Value &parameters) {
		auto combined = geometry.parameters;
		for (const auto &name : parameters.getMemberNames()) combined[name] = parameters[name];
		Json::Value row;
		row["parameters"] = parameters;
		try {
			auto snapshot = compile(source, grammar.apply(combined), specification.seed, row);
			if (snapshot) {
				const auto &scene = *snapshot->generationContext();
				const auto mesh = scene.buildExportMesh(cube.data());
				if (mesh.vertices != fixed_mesh.vertices || mesh.faces != fixed_mesh.faces) {
					throw std::runtime_error("The appearance candidate changed the fitted geometry.");
				}
				double loss = 0;
				for (const auto &[view, mask] : training) loss += color_rmse(render(scene, view->camera), *view, mask);
				loss /= training.size();
				row["color_rmse"] = loss;
				if (!best || loss < best->loss) best = FitCandidate{combined, std::move(snapshot), loss};
			}
		} catch (const std::runtime_error &error) {
			row["error"] = error.what();
		}
		row["ok"] = !row.isMember("error");
		search.append(std::move(row));
	});
	return best;
}

void write_fitted_views(const FitSpecification &specification, const FitCandidate &candidate,
                        const RenderFitView &render, const fs::path &output, Json::Value &report)
{
	const auto &scene = *candidate.scene->generationContext();
	const auto cube = build_base_vertex_data();
	const auto mesh = scene.buildExportMesh(cube.data());
	ExperimentContactSheet sheet(output);
	for (const auto &view : specification.views) {
		auto measured = measure_view(view, scene, mesh, specification.landmark_weight);
		const auto overlay = difference_overlay(view, measured);
		const std::string overlay_name = view.id + "-difference.png";
		write_experiment_png(output / overlay_name, overlay);
		measured.scores["difference_overlay"] = overlay_name;
		sheet.add(view.id + (view.held_out ? " | held out" : " | training"), overlay);
		if (view.image) {
			const auto image = render(scene, view.camera);
			const std::string image_name = view.id + "-fitted.png";
			write_experiment_png(output / image_name, image);
			measured.scores["fitted_image"] = image_name;
			if (measured.scores["intersection_pixels"].asUInt64() > 0) {
				measured.scores["color_rmse"] = color_rmse(image, view, measured.predicted);
			}
		}
		report["views"].append(std::move(measured.scores));
	}
	for (const auto &name : sheet.finish()) report["contact_sheets"].append(name);
}

}

bool fit_grammar_images(const fs::path &grammar_path, const fs::path &views_path, const fs::path &destination,
                        const CompileExperimentScene &compile, const RenderFitView &render, Json::Value &summary)
{
	const auto grammar = fs::absolute(grammar_path).lexically_normal();
	const auto manifest = fs::absolute(views_path).lexically_normal();
	const auto spec = read_specification(manifest);
	const ParameterizedGrammar source(grammar);
	source.checkBindings(spec.geometry);
	source.checkBindings(spec.appearance);
	ExperimentOutput output(destination);

	Json::Value report;
	report["schema_version"] = 1;
	report["grammar"] = grammar.u8string();
	report["reference_manifest"] = manifest.u8string();
	report["seed"] = static_cast<Json::UInt64>(spec.seed);
	report["landmark_weight"] = spec.landmark_weight;
	report["geometry_search"] = Json::Value(Json::arrayValue);
	auto geometry = fit_geometry(spec, source, grammar, compile, report["geometry_search"]);
	std::optional<FitCandidate> appearance;
	if (geometry && !spec.appearance.axes().empty()) {
		report["appearance_search"] = Json::Value(Json::arrayValue);
		appearance = fit_appearance(spec, source, grammar, *geometry, render, compile, report["appearance_search"]);
	}
	const bool success = geometry && (spec.appearance.axes().empty() || appearance.has_value());
	report["ok"] = success;
	summary["report"] = (output.destination() / "fit-report.json").u8string();
	if (success) {
		const auto &selected = appearance ? *appearance : *geometry;
		report["parameters"] = selected.parameters;
		report["geometry_loss"] = geometry->loss;
		if (appearance) report["appearance_color_rmse"] = appearance->loss;
		report["fitted_grammar"] = "fitted.p3d";
		render_files::write_text(output.staging() / "fitted.p3d", source.apply(selected.parameters));
		write_fitted_views(spec, selected, render, output.staging(), report);
		summary["fitted_grammar"] = (output.destination() / "fitted.p3d").u8string();
		summary["parameters"] = selected.parameters;
		summary["views"] = report["views"];
	} else {
		report["error"] = geometry ? "Every appearance candidate failed." : "Every geometry candidate failed.";
		summary["error"] = report["error"];
	}
	render_files::write_json(output.staging() / "fit-report.json", report);
	output.publish();
	return success;
}
