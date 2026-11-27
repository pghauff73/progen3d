#pragma once

#include "audio/model/AudioEventDefinition.h"

#include <cstddef>
#include <optional>
#include <vector>

class SceneGenerationContext;
struct SimulationContact;

using DueAudioEvents = std::vector<const AudioEventDefinition *>;

class SceneAudioEventEvaluator {
public:
	void replaceProgram(const std::vector<AudioEventDefinition> &definitions,
	                    const SceneGenerationContext &scene,
	                    double seconds);
	void bindScene(const SceneGenerationContext &scene);
	void rebase(const SceneGenerationContext &scene, double seconds);

	DueAudioEvents advance(const SceneGenerationContext &scene,
	                       double seconds,
	                       const std::vector<SimulationContact> *physics_contacts = nullptr);
	DueAudioEvents observeCamera(const glm::vec3 &position, bool rebase_camera);

	const std::vector<AudioEventDefinition> &definitions() const { return definitions_; }
	bool hasCollisionTriggers() const { return has_collision_triggers_; }

private:
	enum class ObjectMotion { Fixed, Moving, Sleeping, Stopped, Removed };

	struct TriggerState {
		std::vector<std::size_t> object_instances;
		std::vector<std::size_t> other_instances;
		std::optional<bool> inside_region;
		std::optional<ObjectMotion> motion;
		std::optional<double> last_collision_seconds;
		bool collision_active = false;
	};

	static std::optional<ObjectMotion> objectMotion(
		const SceneGenerationContext &scene, const std::vector<std::size_t> &instances);
	static bool observeRegion(const RegionAudioTrigger &region,
	                          const std::optional<glm::vec3> &position,
	                          std::optional<bool> &inside);

	std::vector<AudioEventDefinition> definitions_;
	std::vector<TriggerState> states_;
	double previous_seconds_ = 0.0;
	bool run_started_ = false;
	bool has_collision_triggers_ = false;
};
