#pragma once

#include "audio/service/SceneAudioEventEvaluator.h"

#include <json/json.h>

class SequenceAudioCues {
public:
	SequenceAudioCues(const std::vector<AudioEventDefinition> &definitions,
	                  const SceneGenerationContext &initial_scene, bool physics);

	bool observesPhysics() const { return observe_physics_; }
	bool observesFrames() const { return observe_object_frames_ || observe_camera_; }
	bool observesCamera() const { return observe_camera_; }

	void observeScene(const SceneGenerationContext &scene, double seconds,
	                  const std::vector<SimulationContact> *contacts);
	void observeFrame(const SceneGenerationContext &scene, double seconds);
	void observeCamera(const glm::vec3 &position, double seconds);

	Json::Value manifest(double end_seconds);

private:
	void add(const AudioEventDefinition &definition, double seconds);

	struct Cue {
		double seconds;
		std::string id;
		const char *trigger;
		SpeechCue speech;
	};

	SceneAudioEventEvaluator evaluator_;
	std::vector<Cue> cues_;
	bool has_events_ = false;
	bool observe_physics_ = false;
	bool observe_object_frames_ = false;
	bool observe_camera_ = false;
};
