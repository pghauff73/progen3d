#include "RenderEffectInputs.h"

#include "ManifestInput.h"
#include "RenderFiles.h"

#include <sstream>
#include <stdexcept>

namespace {
Json::Value take_member(Json::Value &object, const char *name)
{
	Json::Value value;
	object.removeMember(name, &value);
	return value;
}

Json::Value load_input(const std::filesystem::path &path, const char *field, Json::Value &hashes)
{
	const std::string bytes = render_files::read_text(path);
	Json::CharReaderBuilder parser;
	Json::CharReaderBuilder::strictMode(&parser.settings_);
	parser["skipBom"] = true;
	std::istringstream input(bytes);
	Json::Value value;
	std::string error;
	if (!Json::parseFromStream(parser, input, &value, &error)) {
		throw std::runtime_error("Invalid JSON in " + path.u8string() + ": " + error);
	}
	hashes[field] = render_files::sha256_text(bytes);
	return value;
}

Json::Value read_table(const std::filesystem::path &directory, Json::Value &overlay, const char *name,
                       const char *field, const char *hash_field, Json::Value &hashes)
{
	if (!overlay.isMember(field)) return Json::Value(Json::objectValue);
	const auto path = manifest_input::resolve_path(directory, take_member(overlay, field), field);
	const auto table = load_input(path, hash_field, hashes);
	manifest_input::check_fields(table, {"schema_version", name}, hash_field);
	if (!manifest_input::is_unsigned_integer(table["schema_version"]) ||
	    table["schema_version"].asUInt64() != 1 || !table[name].isObject()) {
		throw std::runtime_error(path.u8string() + " requires schema_version 1 and a " + name + " object.");
	}
	return table[name];
}

void merge_channel(Json::Value &effect, const char *field, const Json::Value &table)
{
	if (!effect.isMember(field)) return;
	const auto key = manifest_input::read_string(take_member(effect, field), field);
	if (!table.isMember(key) || !table[key].isObject()) {
		throw std::runtime_error(std::string("Unknown overlay ") + field + " " + key);
	}
	const auto &channel = table[key];
	if (std::string_view(field) == "route") {
		manifest_input::check_fields(channel, {"guides"}, "route " + key);
	} else {
		manifest_input::check_fields(channel, {"signal", "gate", "units", "provenance", "mass_kg",
		                             "vector", "vector_track", "vector_space", "reference_frame",
		                             "vector_scale"}, "telemetry " + key);
	}
	for (const auto &property : channel.getMemberNames()) {
		if (effect.isMember(property)) {
			throw std::runtime_error("An overlay and its " + std::string(field) +
			                         " both define " + property + ".");
		}
		effect[property] = channel[property];
	}
}
}

void resolve_render_effect_inputs(Json::Value &job, const std::filesystem::path &directory)
{
	Json::Value hashes = job.get("input_sha256", Json::Value(Json::objectValue));
	manifest_input::check_fields(hashes, {"rail", "overlays", "routes", "telemetry"}, "input_sha256");
	for (const auto &key : hashes.getMemberNames()) {
		const auto digest = manifest_input::read_string(hashes[key], "input_sha256." + key);
		if (digest.size() != 64 || digest.find_first_not_of("0123456789abcdef") != std::string::npos) {
			throw std::runtime_error("input_sha256." + key + " must be a lowercase SHA-256 digest.");
		}
	}
	if (job.isMember("camera") && job["camera"].isObject() && job["camera"].isMember("rail_file")) {
		auto &camera = job["camera"];
		if (camera.isMember("rail")) throw std::runtime_error("Choose camera.rail or camera.rail_file.");
		const auto path = manifest_input::resolve_path(directory, take_member(camera, "rail_file"), "camera.rail_file");
		camera["rail"] = load_input(path, "rail", hashes);
	}
	auto overlay_directory = directory;
	if (job.isMember("overlay_manifest")) {
		if (job.isMember("overlays")) throw std::runtime_error("Choose overlays or overlay_manifest.");
		const auto path = manifest_input::resolve_path(directory, take_member(job, "overlay_manifest"), "overlay_manifest");
		job["overlays"] = load_input(path, "overlays", hashes);
		overlay_directory = path.parent_path();
	}
	if (job.isMember("overlays")) {
		auto &overlay = job["overlays"];
		if (!overlay.isObject() || !overlay.isMember("effects") || !overlay["effects"].isArray()) {
			throw std::runtime_error("overlays requires an effects array.");
		}
		const auto routes = read_table(overlay_directory, overlay, "routes", "routes", "routes", hashes);
		const auto telemetry = read_table(overlay_directory, overlay, "channels", "telemetry", "telemetry", hashes);
		for (auto &effect : overlay["effects"]) {
			if (!effect.isObject()) throw std::runtime_error("Each overlay effect must be a JSON object.");
			merge_channel(effect, "route", routes);
			merge_channel(effect, "telemetry", telemetry);
		}
	}
	if (!hashes.empty()) job["input_sha256"] = std::move(hashes);
}
