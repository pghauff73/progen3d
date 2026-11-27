#include "SequenceAudioCues.h"

#include <algorithm>
#include <utility>

namespace {

const char *trigger_name(const AudioEventTrigger &trigger)
{
	if (std::holds_alternative<TimeAudioTrigger>(trigger)) return "time";
	if (std::holds_alternative<CollisionAudioTrigger>(trigger)) return "collision";
	if (std::holds_alternative<SimulationAudioTrigger>(trigger)) return "simulation";
	return std::get<RegionAudioTrigger>(trigger).object_id.empty() ? "camera_region" : "object_region";
}

}

SequenceAudioCues::SequenceAudioCues(const std::vector<AudioEventDefinition> &definitions,
                                     const SceneGenerationContext &initial_scene, bool physics)
	: has_events_(!definitions.empty())
{
	std::vector<AudioEventDefinition> observed;
	for (const auto &definition : definitions) {
		if (const auto *time = std::get_if<TimeAudioTrigger>(&definition.trigger)) {
			add(definition, time->seconds);
		} else if (const auto *simulation = std::get_if<SimulationAudioTrigger>(&definition.trigger)) {
			if (!physics) continue;
			if (simulation->event == SimulationAudioEvent::Start) {
				add(definition, 0.0);
			} else {
				observe_physics_ = true;
				observed.push_back(definition);
			}
		} else if (const auto *region = std::get_if<RegionAudioTrigger>(&definition.trigger)) {
			if (region->object_id.empty()) observe_camera_ = true;
			else if (physics) observe_physics_ = true;
			else observe_object_frames_ = true;
			observed.push_back(definition);
		} else if (physics) {
			observe_physics_ = true;
			observed.push_back(definition);
		}
	}
	if (!observed.empty()) evaluator_.replaceProgram(observed, initial_scene, 0.0);
}

void SequenceAudioCues::add(const AudioEventDefinition &definition, double seconds)
{
	cues_.push_back({seconds, definition.id, trigger_name(definition.trigger), definition.speech});
}

void SequenceAudioCues::observeScene(const SceneGenerationContext &scene, double seconds,
                                    const std::vector<SimulationContact> *contacts)
{
	evaluator_.bindScene(scene);
	for (const auto *definition : evaluator_.advance(scene, seconds, contacts)) {
		add(*definition, seconds);
	}
}

void SequenceAudioCues::observeFrame(const SceneGenerationContext &scene, double seconds)
{
	if (observe_object_frames_) observeScene(scene, seconds, nullptr);
}

void SequenceAudioCues::observeCamera(const glm::vec3 &position, double seconds)
{
	for (const auto *definition : evaluator_.observeCamera(position, false)) {
		add(*definition, seconds);
	}
}

Json::Value SequenceAudioCues::manifest(double end_seconds)
{
	if (!has_events_) return Json::Value();
	std::stable_sort(cues_.begin(), cues_.end(), [](const Cue &first, const Cue &second) {
		return first.seconds < second.seconds;
	});
	Json::Value records(Json::arrayValue);
	for (const auto &cue : cues_) {
		if (cue.seconds >= end_seconds) break;
		Json::Value record;
		record["id"] = cue.id;
		record["time_seconds"] = cue.seconds;
		record["trigger"] = cue.trigger;
		record["text"] = cue.speech.text;
		record["speaker_id"] = cue.speech.speaker_id;
		records.append(std::move(record));
	}
	return records;
}
