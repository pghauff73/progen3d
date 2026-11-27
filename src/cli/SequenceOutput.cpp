#include "SequenceOutput.h"

#include "Context.h"
#include "physics/model/SimulationStep.h"

#include <stdexcept>
#include <utility>

namespace {

namespace fs = std::filesystem;

Json::Value identity_json(const TimelineBodyIdentity &identity)
{
	Json::Value result(Json::objectValue);
	result["object_id"] = identity.object_id;
	result["body_id"] = identity.body_id;
	return result;
}

Json::Value matrix_json(const glm::mat4 &matrix)
{
	Json::Value values(Json::arrayValue);
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			values.append(matrix[column][row]);
		}
	}
	return values;
}

Json::Value vector_json(const glm::vec3 &vector)
{
	Json::Value values(Json::arrayValue);
	values.append(vector.x);
	values.append(vector.y);
	values.append(vector.z);
	return values;
}

fs::path sequence_directory(const fs::path &root, const fs::path &checkpoint)
{
	if (!checkpoint.empty()) return checkpoint;
	fs::create_directories(root);
	const auto directory = root / (L"sequence-" + render_files::unique_suffix());
	if (!fs::create_directory(directory)) throw std::runtime_error("The sequence output directory already exists.");
	return directory;
}

}

Json::Value timeline_report(const TimelineSettings &timeline)
{
	Json::Value report(Json::objectValue);
	report["fps"] = timeline.frames_per_second;
	report["frame_count"] = timeline.frame_count;
	report["start_frame"] = timeline.start_frame;
	report["end_frame"] = timeline.end_frame;
	report["physics"] = timeline.physics;
	report["physics_ticks_per_second"] = kSceneSimulationTicksPerSecond;
	if (timeline.shutter) {
		report["shutter"]["frame_fraction"] = timeline.shutter->frame_fraction;
		report["shutter"]["samples"] = timeline.shutter->samples;
	}
	return report;
}

Json::Value sample_transforms(const TimelineRenderSample &sample)
{
	Json::Value result(Json::arrayValue);
	const auto &instances = sample.scene.primitive_instances;
	for (std::size_t index = 0; index < instances.size(); ++index) {
		const auto &instance = instances[index];
		Json::Value value = identity_json(sample.bodies[index]);
		value["primitive_index"] = static_cast<Json::UInt64>(index);
		value["position"] = vector_json(instance.position);
		value["velocity"] = vector_json(instance.velocity);
		value["angular_velocity"] = vector_json(instance.rotational_velocity);
		value["primary_transform"] = matrix_json(instance.primary_transform);
		value["secondary_transform"] = matrix_json(instance.secondary_transform);
		value["immovable"] = instance.immovable;
		value["sleeping"] = instance.sleeping;
		value["removed"] = instance.removed;
		result.append(std::move(value));
	}
	return result;
}

SequenceOutput::SequenceOutput(const fs::path &root, const TimelineSettings &timeline,
                               const Json::Value &job_report, const fs::path &checkpoint)
	: directory_(sequence_directory(root, checkpoint)), pending_contacts_(directory_ / "contacts.jsonl")
{
	contact_stream_.exceptions(std::ios::failbit | std::ios::badbit);
	contact_stream_.open(pending_contacts_.staging(), std::ios::binary);

	manifest_["schema_version"] = 1;
	manifest_["timeline"] = timeline_report(timeline);
	manifest_["timeline"]["export_transforms"] = timeline.export_transforms;
	manifest_["grammar"] = job_report["grammar"];
	manifest_["seed"] = job_report["seed"];
	manifest_["width"] = job_report["width"];
	manifest_["height"] = job_report["height"];
	manifest_["contacts"] = "contacts.jsonl";
	manifest_["frames"] = Json::Value(Json::arrayValue);
}

void SequenceOutput::recordContacts(std::uint64_t tick, const std::vector<TimelineContact> &contacts)
{
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "";
	for (const auto &contact : contacts) {
		Json::Value record;
		record["tick"] = static_cast<Json::UInt64>(tick);
		record["time_seconds"] = static_cast<double>(tick) / kSceneSimulationTicksPerSecond;
		record["first"] = identity_json(contact.first);
		record["second"] = identity_json(contact.second);
		record["peak_impulse"] = contact.peak_impulse;
		contact_stream_ << Json::writeString(writer, record) << '\n';
		++contact_count_;
	}
}

void SequenceOutput::publishFrame(std::uint32_t frame_number, const RenderedFrame &frame, Json::Value metadata)
{
	contact_stream_.flush();
	metadata["seed"] = manifest_["seed"];
	metadata["grammar"] = manifest_["grammar"];
	const auto path = publish_sequence_frame(directory_, frame_number, frame, std::move(metadata));
	indexFrame(frame_number, path, frame.overlay.has_value());
}

void SequenceOutput::reuseFrame(std::uint32_t frame_number, bool overlay)
{
	indexFrame(frame_number, sequence_frame_directory(directory_, frame_number) / "frame.json", overlay);
}

void SequenceOutput::indexFrame(std::uint32_t frame_number, const fs::path &path, bool overlay)
{
	Json::Value entry;
	entry["frame"] = frame_number;
	entry["manifest"] = path.lexically_relative(directory_).generic_u8string();
	entry["checksums"] = (path.parent_path() / "checksums.json").lexically_relative(directory_).generic_u8string();
	if (overlay) {
		entry["emission"] = (path.parent_path() / frame_emission_name).lexically_relative(directory_).generic_u8string();
		entry["effects"] = (path.parent_path() / frame_effects_name).lexically_relative(directory_).generic_u8string();
	}
	manifest_["frames"].append(std::move(entry));
}

fs::path SequenceOutput::finish(std::uint64_t solved_tick, Json::Value audio_cues)
{
	contact_stream_.close();
	pending_contacts_.publish();
	manifest_["last_simulation_tick"] = static_cast<Json::UInt64>(solved_tick);
	manifest_["contact_count"] = static_cast<Json::UInt64>(contact_count_);
	if (!audio_cues.isNull()) manifest_["audio_cues"] = std::move(audio_cues);
	const fs::path manifest = directory_ / "sequence.json";
	render_files::write_json(manifest, manifest_);
	return manifest;
}
