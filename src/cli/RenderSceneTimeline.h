#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class GeneratedSceneSnapshot;
class SceneGenerationContext;
struct SimulationContact;
class TimelineSceneState;
class TimelinePoseSnapshot;

struct TimelineBodyIdentity {
	std::string object_id;
	std::string body_id;
};

struct TimelineContact {
	TimelineBodyIdentity first;
	TimelineBodyIdentity second;
	float peak_impulse = 0.0f;
};

struct TimelineRenderSample {
	const SceneGenerationContext &scene;
	const std::vector<TimelineBodyIdentity> &bodies;
	std::uint64_t simulation_tick = 0;
	double simulation_fraction = 0.0;
};

class RenderSceneTimeline
{
public:
	using EvaluateGrammar = std::function<std::unique_ptr<GeneratedSceneSnapshot>(double time_seconds)>;
	using RecordContacts = std::function<void(std::uint64_t tick, const std::vector<TimelineContact> &contacts)>;
	using ObserveStep = std::function<void(const SceneGenerationContext &, double, const std::vector<SimulationContact> &)>;

	RenderSceneTimeline(std::unique_ptr<GeneratedSceneSnapshot> initial_scene,
	                    EvaluateGrammar evaluate_grammar, bool physics, RecordContacts record_contacts,
	                    ObserveStep observe_step = {});
	~RenderSceneTimeline();

	TimelineRenderSample sample(double time_seconds);
	std::uint64_t solvedTick() const { return solved_tick_; }
	bool hasStaticGeometry() const { return !physics_ && !grammar_uses_time_; }

private:
	void step();

	std::unique_ptr<TimelineSceneState> solved_;
	std::unique_ptr<TimelineSceneState> sampled_;
	std::unique_ptr<TimelinePoseSnapshot> previous_;
	EvaluateGrammar evaluate_grammar_;
	RecordContacts record_contacts_;
	ObserveStep observe_step_;
	std::uint64_t solved_tick_ = 0;
	bool physics_ = false;
	bool grammar_uses_time_ = false;
};
