#include "RenderCheckpoint.h"

#include "FrameOutput.h"
#include "ManifestInput.h"

#include <stdexcept>

namespace {

namespace fs = std::filesystem;
constexpr const char *lock_name = ".render.lock";

fs::path prepare_directory(const fs::path &path)
{
	fs::create_directories(path);
	return fs::canonical(path);
}

bool exists_including_symlink(const fs::path &path)
{
	return fs::exists(fs::symlink_status(path));
}

}

RenderCheckpoint::RenderCheckpoint(const fs::path &directory, bool resume,
                                   const Json::Value &job, std::string_view grammar)
	: directory_(prepare_directory(directory)), lease_(directory_ / lock_name, render_files::LockWait::try_once)
{
	if (!lease_.acquired()) throw std::runtime_error("This render output is already in use: " + directory_.u8string());
	const fs::path record_path = directory_ / "checkpoint.json";
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "";
	Json::Value identity;
	identity["schema_version"] = 1;
	identity["job_sha256"] = render_files::sha256_text(Json::writeString(writer, job));
	identity["grammar_sha256"] = render_files::sha256_text(grammar);

	if (exists_including_symlink(record_path)) {
		if (!resume) throw std::runtime_error("A render checkpoint already exists. Use --resume or a new output directory.");
		if (manifest_input::read_json(record_path) != identity) {
			throw std::runtime_error("This checkpoint belongs to a different render job or grammar. Choose a new output directory.");
		}
		discard_abandoned_frame_staging(directory_);
		for (const char *name : {"contacts.jsonl", "sequence.json", "checkpoint.json"}) {
			render_files::discard_abandoned_writes(directory_ / name);
		}
	} else {
		render_files::discard_abandoned_writes(record_path);
		for (const auto &child : fs::directory_iterator(directory_)) {
			if (child.path().filename() != lock_name) {
				throw std::runtime_error("Choose an empty directory for a new render checkpoint: " + directory_.u8string());
			}
		}
		render_files::write_json(record_path, identity);
	}
}

std::optional<Json::Value> RenderCheckpoint::restoreFrame(std::uint32_t frame, int width, int height, bool layers,
                                                        bool overlay)
{
	const auto path = sequence_frame_directory(directory_, frame);
	const bool present = exists_including_symlink(path);
	if (present) {
		if (auto metadata = verify_sequence_frame(path, frame, width, height, layers, overlay)) return metadata;
	}

	// Retire the index before replacing a frame it might reference.
	const fs::path manifest = directory_ / "sequence.json";
	if (exists_including_symlink(manifest)) {
		render_files::move_into_place(manifest, directory_ / (L".rejected-sequence-" + render_files::unique_suffix() + L".json"));
	}
	if (present) {
		render_files::move_into_place(path, directory_ / (L".rejected-" + path.filename().wstring() + L"-" + render_files::unique_suffix()));
	}
	return std::nullopt;
}
