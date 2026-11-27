#include "VideoEdit.h"
#include "ManifestInput.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <map>
#include <stdexcept>
#include <string_view>

namespace {

namespace fs = std::filesystem;
using manifest_input::is_unsigned_integer;
using manifest_input::read_json;
using manifest_input::resolve_path;

void check_keys(const Json::Value &object, std::initializer_list<std::string_view> allowed, const char *field)
{
	if (!object.isObject()) throw std::runtime_error(std::string(field) + " must be an object.");
	for (const auto &key : object.getMemberNames()) {
		if (std::find(allowed.begin(), allowed.end(), key) == allowed.end()) {
			throw std::runtime_error(std::string(field) + " contains an unknown field: " + key);
		}
	}
}

void check_version(const Json::Value &root, const fs::path &path)
{
	if (!root.isObject() || !is_unsigned_integer(root["schema_version"]) ||
	    root["schema_version"].asUInt64() != 1) {
		throw std::runtime_error("Expected schema_version 1 in " + path.u8string());
	}
}

const Json::Value &read_array(const Json::Value &object, const char *key)
{
	if (!object[key].isArray()) throw std::runtime_error(std::string(key) + " must be an array.");
	return object[key];
}

std::uint32_t integer(const Json::Value &value, std::uint32_t upper, const char *field)
{
	if (!is_unsigned_integer(value) || value.asUInt64() > upper) {
		throw std::runtime_error(std::string(field) + " must be an integer from 0 to " + std::to_string(upper) + ".");
	}
	return value.asUInt();
}

std::string text(const Json::Value &value, const char *field)
{
	if (!value.isString()) throw std::runtime_error(std::string(field) + " must be text.");
	auto result = value.asString();
	if (result.empty() || result.find('\0') != std::string::npos) {
		throw std::runtime_error(std::string(field) + " must be nonempty text without NUL characters.");
	}
	return result;
}

double nonnegative(const Json::Value &value, const char *field)
{
	if (!value.isNumeric() || !std::isfinite(value.asDouble()) || value.asDouble() < 0.0) {
		throw std::runtime_error(std::string(field) + " must be a finite nonnegative number.");
	}
	return value.asDouble();
}

struct SavedSequence {
	explicit SavedSequence(const fs::path &path) : manifest(read_json(path))
	{
		check_version(manifest, path);
		if (!manifest["timeline"].isObject()) throw std::runtime_error("Sequence has no timeline: " + path.u8string());
		fps = static_cast<int>(integer(manifest["timeline"]["fps"], 120, "sequence fps"));
		width = static_cast<int>(integer(manifest["width"], 4096, "sequence width"));
		height = static_cast<int>(integer(manifest["height"], 4096, "sequence height"));
		if (fps == 0 || width == 0 || height == 0 || static_cast<std::int64_t>(width) * height > 8388608) {
			throw std::runtime_error("Invalid sequence size or frame rate: " + path.u8string());
		}
		for (const auto &frame : read_array(manifest, "frames")) {
			if (!frame.isObject()) throw std::runtime_error("Invalid frame entry in " + path.u8string());
			const auto number = integer(frame["frame"], std::numeric_limits<std::uint32_t>::max(), "frame number");
			if (!frames.emplace(number, resolve_path(path.parent_path(), frame["manifest"], "frame manifest")).second) {
				throw std::runtime_error("Duplicate frame number in " + path.u8string());
			}
		}
	}

	int fps;
	int width;
	int height;
	Json::Value manifest;
	std::map<std::uint32_t, fs::path> frames;
};

fs::path frame_image(const SavedSequence &sequence, std::uint32_t number)
{
	auto found = sequence.frames.find(number);
	if (found == sequence.frames.end()) {
		throw std::runtime_error("Cut references unsaved frame " + std::to_string(number) + ".");
	}
	const auto &path = found->second;
	const auto metadata = read_json(path);
	check_version(metadata, path);
	if (!is_unsigned_integer(metadata["frame"]) || metadata["frame"].asUInt64() != number ||
	    !metadata["files"].isObject()) {
		throw std::runtime_error("Frame manifest does not match its sequence: " + path.u8string());
	}
	return resolve_path(path.parent_path(), metadata["files"]["preview"], "preview image");
}

std::vector<EditText> read_overlays(const Json::Value &edit, const char *field, std::uint32_t frame_count)
{
	std::vector<EditText> result;
	if (!edit.isMember(field)) return result;
	for (const auto &entry : read_array(edit, field)) {
		check_keys(entry, {"start_frame", "end_frame", "text"}, field);
		const auto start = integer(entry["start_frame"], frame_count, "overlay start_frame");
		const auto end = integer(entry["end_frame"], frame_count, "overlay end_frame");
		if (start >= end) throw std::runtime_error("Overlay end_frame must be after start_frame.");
		result.push_back({start, end, text(entry["text"], "overlay text")});
	}
	std::stable_sort(result.begin(), result.end(), [](const EditText &first, const EditText &second) {
		return first.start_frame < second.start_frame;
	});
	return result;
}

}

VideoEdit::VideoEdit(const fs::path &manifest_path)
{
	const auto path = fs::absolute(manifest_path).lexically_normal();
	const Json::Value edit = read_json(path);
	check_version(edit, path);
	check_keys(edit, {"schema_version", "cuts", "titles", "captions", "narration", "use_audio_events"}, "Edit manifest");
	if (edit.isMember("use_audio_events") && !edit["use_audio_events"].isBool()) {
		throw std::runtime_error("use_audio_events must be true or false.");
	}
	const bool use_audio_events = !edit.isMember("use_audio_events") || edit["use_audio_events"].asBool();
	std::map<fs::path, SavedSequence> sequences;
	for (const auto &entry : read_array(edit, "cuts")) {
		check_keys(entry, {"sequence", "start_frame", "end_frame"}, "Cut");
		const auto sequence_path = resolve_path(path.parent_path(), entry["sequence"], "cut sequence");
		const auto &sequence = sequences.try_emplace(sequence_path, sequence_path).first->second;
		if (cuts.empty()) {
			fps = sequence.fps;
			width = sequence.width;
			height = sequence.height;
		} else if (fps != sequence.fps || width != sequence.width || height != sequence.height) {
			throw std::runtime_error("All cuts must use the same render size and frame rate.");
		}
		const auto start = integer(entry["start_frame"], std::numeric_limits<std::uint32_t>::max(), "cut start_frame");
		const auto end = integer(entry["end_frame"], std::numeric_limits<std::uint32_t>::max(), "cut end_frame");
		if (start >= end) throw std::runtime_error("Cut end_frame must be after start_frame.");
		if (end - start > sequence.frames.size() ||
		    frames.size() + (end - start) > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
			throw std::runtime_error("Cut references unsaved frames or exceeds the supported frame count.");
		}
		const auto output_start = static_cast<std::uint32_t>(frames.size());
		cuts.push_back({sequence_path, start, end, output_start});
		for (auto frame = start; frame < end; ++frame) {
			frames.push_back(frame_image(sequence, frame));
		}
		if (!use_audio_events || !sequence.manifest.isMember("audio_cues")) continue;
		for (const auto &cue : read_array(sequence.manifest, "audio_cues")) {
			if (!cue.isObject()) throw std::runtime_error("Invalid recorded audio cue in " + sequence_path.u8string());
			const double seconds = nonnegative(cue["time_seconds"], "audio cue time_seconds");
			const double cut_start = static_cast<double>(start) / fps;
			const double cut_end = static_cast<double>(end) / fps;
			if (seconds < cut_start || seconds >= cut_end) continue;
			const double output_seconds = static_cast<double>(output_start) / fps + (seconds - cut_start);
			const double output_end = static_cast<double>(frames.size()) / fps;
			narration.push_back({output_seconds, output_end, 1.0, std::nullopt,
			                     {text(cue["text"], "audio cue text"),
			                      static_cast<int>(integer(cue["speaker_id"], std::numeric_limits<int>::max(), "speaker_id"))},
			                     text(cue["id"], "audio cue id"), text(cue["trigger"], "audio cue trigger")});
		}
	}
	if (frames.empty()) throw std::runtime_error("An edit requires at least one cut.");
	const auto frame_count = static_cast<std::uint32_t>(frames.size());
	titles = read_overlays(edit, "titles", frame_count);
	captions = read_overlays(edit, "captions", frame_count);
	if (edit.isMember("narration")) {
		for (const auto &entry : read_array(edit, "narration")) {
			check_keys(entry, {"start_frame", "wav", "gain"}, "Narration");
			const auto start = integer(entry["start_frame"], frame_count - 1, "narration start_frame");
			const auto wav = resolve_path(path.parent_path(), entry["wav"], "narration WAV");
			const double gain = entry.isMember("gain") ? nonnegative(entry["gain"], "narration gain") : 1.0;
			narration.push_back({static_cast<double>(start) / fps, duration(), gain, wav, {}, {}, "wav"});
		}
	}
	std::stable_sort(narration.begin(), narration.end(), [](const EditNarration &first, const EditNarration &second) {
		return first.start_seconds < second.start_seconds;
	});
}
