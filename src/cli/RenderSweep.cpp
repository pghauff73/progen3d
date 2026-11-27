#include "RenderSweep.h"

#include "ExperimentImages.h"
#include "ManifestInput.h"
#include "RenderFiles.h"

#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace {

namespace fs = std::filesystem;

std::string image_id(std::size_t index)
{
	std::ostringstream label;
	label << "variant-" << std::setfill('0') << std::setw(3) << index;
	return label.str();
}

std::string variant_label(const std::string &id, const ParameterGrid &grid,
                          const Json::Value &values, std::optional<float> aperture)
{
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "";
	writer["precision"] = 9;
	std::string label = id;
	if (aperture) {
		std::ostringstream formatted;
		formatted << *aperture;
		label += "\naperture = " + formatted.str();
	}
	for (const auto &axis : grid.axes()) {
		const auto &value = values[axis.name];
		label += "\n" + axis.name + " = " + (value.isString() ? value.asString() : Json::writeString(writer, value));
	}
	return label;
}

}

RenderSweep::RenderSweep(const fs::path &manifest)
	: manifest_(fs::absolute(manifest).lexically_normal())
{
	const auto root = manifest_input::read_json(manifest_);
	manifest_input::check_fields(root, {"schema_version", "job", "parameters", "aperture_radii", "object"}, "sweep");
	if (root["schema_version"] != 1) throw std::runtime_error("Sweep requires schema_version 1.");
	job_ = manifest_input::resolve_path(manifest_.parent_path(), root["job"], "sweep.job");
	parameters_ = ParameterGrid::parse(root["parameters"], "sweep.parameters");
	if (root.isMember("object")) object_ = manifest_input::read_string(root["object"], "sweep.object");
	if (root.isMember("aperture_radii")) {
		const auto &radii = root["aperture_radii"];
		if (!radii.isArray() || radii.empty() || radii.size() > 4096u / parameters_.size()) {
			throw std::runtime_error("aperture_radii must be a nonempty array within the 4096-variant sweep limit.");
		}
		std::set<float> unique;
		for (const auto &value : radii) {
			const auto radius = manifest_input::read_number(value, "aperture_radii");
			if (radius < 0.0 || radius > std::numeric_limits<float>::max() || !unique.insert(static_cast<float>(radius)).second) {
				throw std::runtime_error("Sweep apertures must be distinct nonnegative radii.");
			}
			apertures_.emplace_back(static_cast<float>(radius));
		}
	} else {
		apertures_.push_back(std::nullopt);
	}
	if (parameters_.axes().empty() && !variesAperture()) throw std::runtime_error("Sweep needs a grammar or aperture axis.");
}

bool RenderSweep::run(const fs::path &grammar, const fs::path &destination,
                      const Json::Value &capture_settings, const RenderVariant &render, Json::Value &summary) const
{
	const ParameterizedGrammar source(grammar);
	source.checkBindings(parameters_);
	ExperimentOutput output(destination);
	ExperimentContactSheet sheet(output.staging());
	Json::Value report;
	report["schema_version"] = 1;
	report["sweep"] = manifest_.u8string();
	report["job"] = job_.u8string();
	report["grammar"] = grammar.u8string();
	report["capture"] = capture_settings;
	if (object_) report["object"] = *object_;
	report["variants"] = Json::Value(Json::arrayValue);
	std::size_t index = 0;
	std::size_t failed = 0;
	parameters_.forEach([&](const Json::Value &parameters) {
		const auto text = source.apply(parameters);
		for (const auto aperture : apertures_) {
			const auto id = image_id(++index);
			Json::Value variant;
			variant["id"] = id;
			variant["parameters"] = parameters;
			if (aperture) variant["aperture_radius"] = *aperture;

			PreviewImage image;
			try {
				image = render(text, aperture, variant);
			} catch (const std::runtime_error &error) {
				variant["error"] = error.what();
			}
			variant["ok"] = image.isValid() && !variant.isMember("error");
			if (variant["ok"].asBool()) {
				const auto name = id + ".png";
				write_experiment_png(output.staging() / name, image);
				variant["preview"] = name;
				const auto &bytes = image.rgbPixels();
				variant["rgb_sha256"] = render_files::sha256_text(
					std::string_view(reinterpret_cast<const char *>(bytes.data()), bytes.size()));
			} else {
				++failed;
				if (!variant.isMember("error")) variant["error"] = "No render was produced.";
				image = {};
			}
			sheet.add(variant_label(id, parameters_, parameters, aperture), image);
			report["variants"].append(std::move(variant));
		}
	});
	report["ok"] = failed == 0;
	report["rendered_variants"] = static_cast<Json::UInt64>(index - failed);
	report["failed_variants"] = static_cast<Json::UInt64>(failed);
	summary = report;
	summary.removeMember("variants");
	summary["metrics"] = (output.destination() / "metrics.json").u8string();
	for (const auto &name : sheet.finish()) {
		report["contact_sheets"].append(name);
		summary["contact_sheets"].append((output.destination() / name).u8string());
	}
	render_files::write_json(output.staging() / "metrics.json", report);
	output.publish();
	return failed == 0;
}
