#include "FrameOutput.h"
#include "ManifestInput.h"
#include "RenderFiles.h"

#include "editor/service/PreviewFramebufferCaptureService.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <limits>
#include <stdexcept>

namespace {

namespace fs = std::filesystem;
constexpr const char *preview_name = "preview.ppm";
constexpr const char *sharp_name = "sharp-color.ppm";
constexpr const char *depth_name = "linear-depth.pfm";
constexpr const char *objects_name = "object-mask.ppm";
constexpr const char *coverage_name = "foreground-coverage.pgm";
constexpr const char *manifest_name = "frame.json";
constexpr const char *checksums_name = "checksums.json";

struct FrameArtifact {
	const char *key;
	const char *name;
};

constexpr FrameArtifact layer_artifacts[] = {
	{"color", sharp_name}, {"linear_depth", depth_name},
	{"object_mask", objects_name}, {"foreground_coverage", coverage_name}
};

constexpr FrameArtifact overlay_artifacts[] = {
	{"emission", frame_emission_name}, {"effects", frame_effects_name}
};

void discard_staging(const fs::path &staging)
{
	std::error_code ignored;
	for (const char *file : {preview_name, sharp_name, depth_name, objects_name, coverage_name,
	                        frame_emission_name, frame_effects_name, manifest_name, checksums_name}) {
		fs::remove(staging / file, ignored);
	}
	fs::remove(staging, ignored);
}

class PendingFrameDirectory
{
public:
	PendingFrameDirectory(const fs::path &root, const fs::path &destination)
	{
		fs::create_directories(root);
		const std::wstring name = L"frame-" + render_files::unique_suffix();
		output_ = destination.empty() ? root / name : root / destination;
		staging_ = root / (L"." + name + L".tmp");
		if (!fs::create_directory(staging_)) {
			throw std::runtime_error("A staged frame directory already exists.");
		}
	}

	PendingFrameDirectory(const PendingFrameDirectory &) = delete;
	PendingFrameDirectory &operator=(const PendingFrameDirectory &) = delete;

	~PendingFrameDirectory()
	{
		if (!staging_.empty()) discard_staging(staging_);
	}

	fs::path stage(const char *name) const { return staging_ / name; }
	fs::path output(const char *name) const { return output_ / name; }

	void publish()
	{
		render_files::move_into_place(staging_, output_);
		staging_.clear();
	}

private:
	fs::path staging_;
	fs::path output_;
};

std::ofstream open_layer(const fs::path &path)
{
	std::ofstream output(path, std::ios::binary);
	if (!output) {
		throw std::runtime_error("Cannot open layer output " + path.u8string());
	}
	output.exceptions(std::ios::failbit | std::ios::badbit);
	return output;
}

void write_checksums(const PendingFrameDirectory &pending, const Json::Value &files)
{
	Json::Value record;
	record["schema_version"] = 1;
	auto &hashes = record["sha256"];
	const auto add = [&](const std::string &name) {
		const auto path = pending.stage(name.c_str());
		render_files::flush_file(path);
		hashes[name] = render_files::sha256_file(path);
	};
	add(manifest_name);
	for (const auto &file : files) add(file.asString());

	const auto path = pending.stage(checksums_name);
	auto output = open_layer(path);
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "  ";
	output << Json::writeString(writer, record) << '\n';
	output.close();
	render_files::flush_file(path);
}

std::string pfm_header(int width, int height, int channels)
{
	const char *format = channels == 1 ? "Pf\n" : "PF\n";
	return format + std::to_string(width) + ' ' + std::to_string(height) + "\n-1.0\n";
}

void write_float_image(const fs::path &path, const std::vector<float> &pixels, int width, int height, int channels)
{
	static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559,
	              "PFM requires 32-bit IEEE floats.");
	const auto row_size = static_cast<std::size_t>(width) * channels;
	if (pixels.size() != row_size * height) {
		throw std::runtime_error("Float layer dimensions do not match the frame: " + path.u8string());
	}
	auto output = open_layer(path);
	// PFM uses a negative scale for little-endian data and rows ordered bottom to top.
	output << pfm_header(width, height, channels);
	for (int row = height - 1; row >= 0; --row) {
		const auto *first_pixel = pixels.data() + static_cast<std::size_t>(row) * row_size;
		output.write(reinterpret_cast<const char *>(first_pixel),
		             static_cast<std::streamsize>(row_size * sizeof(float)));
	}
	output.close();
}

bool has_emission_dimensions(const fs::path &path, int width, int height)
{
	const auto header = pfm_header(width, height, 3);
	const auto image_bytes = static_cast<std::uintmax_t>(width) * height * 3 * sizeof(float);
	if (fs::file_size(path) != header.size() + image_bytes) return false;
	std::ifstream input(path, std::ios::binary);
	std::string actual(header.size(), '\0');
	return input.read(actual.data(), static_cast<std::streamsize>(actual.size())) && actual == header;
}

bool is_effect_record(const Json::Value &records)
{
	return records.isObject() || records.isArray();
}

void write_overlay(const PendingFrameDirectory &pending, const RenderFrameOverlay &overlay, int width, int height)
{
	if (!is_effect_record(overlay.records)) {
		throw std::runtime_error("An emission layer requires a JSON object or array of effect records.");
	}
	if (!std::all_of(overlay.emission_rgb.begin(), overlay.emission_rgb.end(),
	                 [](float channel) { return std::isfinite(channel); })) {
		throw std::runtime_error("Emission layer contains a non-finite channel.");
	}
	write_float_image(pending.stage(frame_emission_name), overlay.emission_rgb, width, height, 3);
	auto output = open_layer(pending.stage(frame_effects_name));
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "  ";
	output << Json::writeString(writer, overlay.records) << '\n';
	output.close();
}

void write_coverage(const fs::path &path, const std::vector<std::uint8_t> &coverage, int width, int height)
{
	auto output = open_layer(path);
	output << "P5\n" << width << ' ' << height << "\n255\n";
	output.write(reinterpret_cast<const char *>(coverage.data()), static_cast<std::streamsize>(coverage.size()));
	output.close();
}

std::uint64_t write_preview(const fs::path &path, const PreviewImage &image)
{
	const auto evidence = PreviewFramebufferCaptureService().writePreview(image, path);
	if (!evidence.succeeded()) {
		throw std::runtime_error(evidence.message());
	}
	return evidence.pixelHash();
}

Json::Value write_object_masks(const fs::path &path, const RenderFrameLayers &layers, int width, int height)
{
	const auto &ids = layers.geometry.object_ids;
	std::vector<std::uint8_t> pixels(ids.size() * 3);
	std::vector<std::size_t> counts(layers.labels.size(), 0);
	for (std::size_t index = 0; index < ids.size(); ++index) {
		const std::uint32_t label = ids[index];
		++counts.at(label);
		pixels[index * 3] = static_cast<std::uint8_t>((label >> 16) & 255u);
		pixels[index * 3 + 1] = static_cast<std::uint8_t>((label >> 8) & 255u);
		pixels[index * 3 + 2] = static_cast<std::uint8_t>(label & 255u);
	}
	write_preview(path, PreviewImage(width, height, std::move(pixels)));
	Json::Value legend(Json::arrayValue);
	for (std::size_t index = 0; index < layers.labels.size(); ++index) {
		const auto &label = layers.labels[index];
		Json::Value entry;
		entry["id"] = static_cast<Json::UInt>(index);
		entry["pixels"] = static_cast<Json::UInt64>(counts[index]);
		if (!label.object.empty()) {
			entry["object"] = label.object;
		} else if (label.primitive_index) {
			entry["primitive_index"] = static_cast<Json::UInt64>(*label.primitive_index);
		}
		legend.append(std::move(entry));
	}
	return legend;
}


struct PublishedFrame {
	Json::Value paths;
	std::string hash;
};

PublishedFrame write_frame_directory(const fs::path &root, const fs::path &destination,
                                     const RenderedFrame &frame, Json::Value manifest)
{
	const int width = frame.color.width();
	const int height = frame.color.height();
	PendingFrameDirectory pending(root, destination);
	const auto hash = write_preview(pending.stage(preview_name), frame.color);
	manifest["schema_version"] = 1;
	manifest["width"] = width;
	manifest["height"] = height;
	manifest["pixel_hash_fnv1a64"] = std::to_string(hash);

	Json::Value paths;
	const auto add_file = [&](const char *key, const char *name) {
		manifest["files"][key] = name;
		paths[key] = pending.output(name).u8string();
	};
	add_file("preview", preview_name);
	if (frame.layers) {
		const auto &layers = *frame.layers;
		write_preview(pending.stage(sharp_name), layers.sharp_color);
		write_float_image(pending.stage(depth_name), layers.geometry.linear_depth, width, height, 1);
		write_coverage(pending.stage(coverage_name), layers.foreground_coverage, width, height);
		manifest["object_ids"] = write_object_masks(pending.stage(objects_name), layers, width, height);
		for (const auto &artifact : layer_artifacts) add_file(artifact.key, artifact.name);
	}
	if (frame.overlay) {
		write_overlay(pending, *frame.overlay, width, height);
		for (const auto &artifact : overlay_artifacts) add_file(artifact.key, artifact.name);
	}
	paths["manifest"] = pending.output(manifest_name).u8string();

	auto output = open_layer(pending.stage(manifest_name));
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "  ";
	output << Json::writeString(writer, manifest) << '\n';
	output.close();
	if (!destination.empty() || frame.overlay) {
		write_checksums(pending, manifest["files"]);
		paths["checksums"] = pending.output(checksums_name).u8string();
	}
	pending.publish();
	return {std::move(paths), std::to_string(hash)};
}

}

void publish_layered_frame(const fs::path &output_directory, const RenderedFrame &frame, Json::Value &report)
{
	Json::Value manifest;
	manifest["seed"] = report["seed"];
	manifest["grammar"] = report["grammar"];
	if (report.isMember("camera")) {
		manifest["camera"] = report["camera"];
	}
	auto published = write_frame_directory(output_directory, {}, frame, std::move(manifest));
	report["image"] = published.paths["preview"];
	report["layers"] = std::move(published.paths);
	report["pixel_hash_fnv1a64"] = std::move(published.hash);
}

fs::path publish_sequence_frame(const fs::path &output_directory, std::uint32_t frame_number,
                                const RenderedFrame &frame, Json::Value metadata)
{
	const auto directory = sequence_frame_directory(output_directory, frame_number);
	const auto published = write_frame_directory(output_directory, directory.filename(), frame, std::move(metadata));
	return fs::u8path(published.paths["manifest"].asString());
}

fs::path sequence_frame_directory(const fs::path &output_directory, std::uint32_t frame_number)
{
	std::ostringstream name;
	name << "frame-" << std::setfill('0') << std::setw(6) << frame_number;
	return output_directory / name.str();
}

std::optional<Json::Value> verify_sequence_frame(const fs::path &directory, std::uint32_t frame_number,
                                               int width, int height, bool layers, bool overlay)
{
	try {
		const auto checksums = manifest_input::read_json(directory / checksums_name);
		if (!checksums.isObject() || checksums["schema_version"] != 1) return std::nullopt;
		const auto &hashes = checksums["sha256"];
		const auto matches = [&](const char *name) {
			const auto path = directory / name;
			return hashes[name].isString() && fs::is_regular_file(fs::symlink_status(path)) &&
			       hashes[name].asString() == render_files::sha256_file(path);
		};
		const unsigned int file_count = 1u + (layers ? 4u : 0u) + (overlay ? 2u : 0u);
		if (!hashes.isObject() || hashes.size() != file_count + 1 || !matches(manifest_name) || !matches(preview_name)) {
			return std::nullopt;
		}
		const auto frame = manifest_input::read_json(directory / manifest_name);
		if (!frame.isObject() || frame["schema_version"] != 1 ||
		    !manifest_input::is_unsigned_integer(frame["frame"]) || frame["frame"].asUInt64() != frame_number ||
		    frame["width"] != width || frame["height"] != height) return std::nullopt;
		const auto &files = frame["files"];
		if (!files.isObject() || files.size() != file_count || files["preview"] != preview_name) return std::nullopt;
		if (layers) {
			for (const auto &artifact : layer_artifacts) {
				if (files[artifact.key] != artifact.name || !matches(artifact.name)) return std::nullopt;
			}
		}
		if (overlay) {
			for (const auto &artifact : overlay_artifacts) {
				if (files[artifact.key] != artifact.name || !matches(artifact.name)) return std::nullopt;
			}
			if (!has_emission_dimensions(directory / frame_emission_name, width, height) ||
			    !is_effect_record(manifest_input::read_json(directory / frame_effects_name))) return std::nullopt;
		}
		return frame;
	} catch (const std::runtime_error &) {
		return std::nullopt;
	}
}

void discard_abandoned_frame_staging(const fs::path &directory)
{
	for (const auto &child : fs::directory_iterator(directory)) {
		const auto name = child.path().filename().wstring();
		if (name.size() <= 11 || name.compare(0, 7, L".frame-") != 0 || child.path().extension() != L".tmp") continue;
		const auto suffix = name.substr(7, name.size() - 11);
		if (!std::all_of(suffix.begin(), suffix.end(), [](wchar_t ch) { return (ch >= L'0' && ch <= L'9') || ch == L'-'; })) continue;
		const DWORD attributes = GetFileAttributesW(child.path().c_str());
		if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) &&
		    !(attributes & FILE_ATTRIBUTE_REPARSE_POINT)) discard_staging(child.path());
	}
}
