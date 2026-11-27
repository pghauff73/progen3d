#pragma once

#include <glm/vec3.hpp>

#include <string>
#include <variant>

struct SpeechCue {
	std::string text;
	int speaker_id = 0;
};

struct TimeAudioTrigger {
	double seconds = 0.0;
};

enum class SimulationAudioEvent { Start, Sleep, Wake, Stop, Remove };

struct SimulationAudioTrigger {
	SimulationAudioEvent event = SimulationAudioEvent::Start;
	std::string object_id;
};

struct CollisionAudioTrigger {
	std::string first_object_id;
	std::string second_object_id;
	float minimum_impulse = 0.25f;
};

enum class AudioRegionTransition { Enter, Exit };

struct RegionAudioTrigger {
	// An empty object ID selects the preview camera.
	std::string object_id;
	AudioRegionTransition transition = AudioRegionTransition::Enter;
	glm::vec3 center{0.0f};
	float radius = 1.0f;
};

using AudioEventTrigger = std::variant<TimeAudioTrigger, SimulationAudioTrigger,
                                      CollisionAudioTrigger, RegionAudioTrigger>;

struct AudioEventDefinition {
	std::string id;
	AudioEventTrigger trigger;
	SpeechCue speech;
};
