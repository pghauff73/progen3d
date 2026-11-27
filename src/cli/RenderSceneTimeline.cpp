#include "RenderSceneTimeline.h"

#include "Context.h"
#include "GrammarRuntime.h"
#include "editor/model/GeneratedSceneSnapshot.h"
#include "physics/model/SimulationStep.h"
#include "spatial/model/SpatialBuildingModel.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace {

std::vector<TimelineBodyIdentity> identify_bodies(const SceneGenerationContext &scene)
{
	std::vector<TimelineBodyIdentity> identities(scene.primitive_instances.size());
	if (const auto &model = scene.spatialBuildingModel()) {
		for (const auto &binding : model->geometryBindings()) {
			auto &identity = identities.at(binding.primitiveInstanceIndex());
			if (identity.object_id.empty()) {
				identity.object_id = binding.objectId().value();
			}
		}
	}
	std::unordered_map<std::string, std::size_t> occurrences;
	for (std::size_t index = 0; index < identities.size(); ++index) {
		auto &identity = identities[index];
		const auto &instance = scene.primitive_instances[index];
		const std::string source = instance.type + "@" + std::to_string(instance.source_start_line + 1) +
		                           ":" + std::to_string(instance.source_start_column + 1);
		const std::string prefix = identity.object_id.empty()
			? "primitive/" + source : "object/" + identity.object_id + "/" + source;
		identity.body_id = prefix + "/" + std::to_string(++occurrences[prefix]);
		if (identity.object_id.empty()) {
			identity.object_id = identity.body_id;
		}
	}
	return identities;
}

struct BodyPose {
	glm::mat4 primary;
	glm::mat4 secondary;
	glm::vec3 velocity;
	glm::vec3 angular_velocity;
	glm::vec3 rotation;
	float sleep_timer;
	bool active;
	bool removed;
	bool sleeping;

	static BodyPose capture(const PrimitiveInstance &instance)
	{
		return {instance.primary_transform, instance.secondary_transform,
		        instance.velocity, instance.rotational_velocity, instance.rotation_degrees,
		        instance.sleep_timer, instance.simulation_active, instance.removed, instance.sleeping};
	}

	void apply(PrimitiveInstance &instance) const
	{
		instance.primary_transform = primary;
		instance.secondary_transform = secondary;
		instance.position = glm::vec3(primary[3]);
		instance.rotation_degrees = rotation;
		instance.velocity = velocity;
		instance.rotational_velocity = angular_velocity;
		instance.sleep_timer = sleep_timer;
		instance.simulation_active = active;
		instance.removed = removed;
		instance.sleeping = sleeping;
		instance.has_previous_render_state = false;
		instance.collision_geometry_dirty = true;
	}

	BodyPose interpolate(const BodyPose &next, float weight) const
	{
		if (removed || next.removed) {
			return *this;
		}
		BodyPose result = *this;
		for (int column = 0; column < 4; ++column) {
			result.primary[column] = glm::mix(primary[column], next.primary[column], weight);
			result.secondary[column] = glm::mix(secondary[column], next.secondary[column], weight);
		}
		result.velocity = glm::mix(velocity, next.velocity, weight);
		result.angular_velocity = glm::mix(angular_velocity, next.angular_velocity, weight);
		return result;
	}
};

double tick_time(std::uint64_t tick)
{
	return static_cast<double>(tick) / kSceneSimulationTicksPerSecond;
}

}

class TimelineSceneState
{
public:
	explicit TimelineSceneState(std::unique_ptr<GeneratedSceneSnapshot> snapshot)
		: snapshot_(std::move(snapshot)), bodies(identify_bodies(scene()))
	{
		for (std::size_t index = 0; index < bodies.size(); ++index) {
			by_body_.emplace(bodies[index].body_id, index);
		}
	}

	SceneGenerationContext &scene() const { return *snapshot_->generationContext(); }
	const GrammarDocument *grammar() const { return snapshot_->grammarDocument(); }

	const PrimitiveInstance *find(const std::string &body_id) const
	{
		const auto match = by_body_.find(body_id);
		return match == by_body_.end() ? nullptr : &scene().primitive_instances[match->second];
	}

private:
	std::unique_ptr<GeneratedSceneSnapshot> snapshot_;

public:
	const std::vector<TimelineBodyIdentity> bodies;

private:
	std::unordered_map<std::string, std::size_t> by_body_;
};

class TimelinePoseSnapshot
{
public:
	void capture(const TimelineSceneState &state)
	{
		poses_.clear();
		const auto &instances = state.scene().primitive_instances;
		for (std::size_t index = 0; index < instances.size(); ++index) {
			if (!instances[index].immovable) {
				poses_.emplace(state.bodies[index].body_id, BodyPose::capture(instances[index]));
			}
		}
	}

	const BodyPose *find(const std::string &body_id) const
	{
		const auto pose = poses_.find(body_id);
		return pose == poses_.end() ? nullptr : &pose->second;
	}

	void carryTo(TimelineSceneState &next) const
	{
		auto &instances = next.scene().primitive_instances;
		for (std::size_t index = 0; index < instances.size(); ++index) {
			if (!instances[index].immovable) {
				if (const auto *pose = find(next.bodies[index].body_id)) {
					pose->apply(instances[index]);
				}
			}
		}
	}

private:
	std::unordered_map<std::string, BodyPose> poses_;
};

RenderSceneTimeline::RenderSceneTimeline(std::unique_ptr<GeneratedSceneSnapshot> initial_scene,
                                         EvaluateGrammar evaluate_grammar, bool physics,
                                         RecordContacts record_contacts, ObserveStep observe_step)
	: solved_(std::make_unique<TimelineSceneState>(std::move(initial_scene))),
	  previous_(std::make_unique<TimelinePoseSnapshot>()),
	  evaluate_grammar_(std::move(evaluate_grammar)),
	  record_contacts_(std::move(record_contacts)),
	  observe_step_(std::move(observe_step)),
	  physics_(physics),
	  grammar_uses_time_(grammarUsesTime(solved_->grammar()))
{
}

RenderSceneTimeline::~RenderSceneTimeline() = default;

void RenderSceneTimeline::step()
{
	previous_->capture(*solved_);
	if (grammar_uses_time_) {
		// Static geometry and new bodies come from this tick's grammar. A
		// surviving movable body keeps its solved pose and velocity.
		auto next = std::make_unique<TimelineSceneState>(evaluate_grammar_(tick_time(solved_tick_ + 1)));
		previous_->carryTo(*next);
		solved_ = std::move(next);
	}
	solved_->scene().time = static_cast<float>(tick_time(solved_tick_));
	std::vector<SimulationContact> native_contacts;
	solved_->scene().stepSimulation(kSceneSimulationStepSeconds, &native_contacts);
	++solved_tick_;
	solved_->scene().time = static_cast<float>(tick_time(solved_tick_));
	if (observe_step_) observe_step_(solved_->scene(), tick_time(solved_tick_), native_contacts);

	if (!native_contacts.empty()) {
		// The solver can emit the same pair on several iterations in one tick.
		std::map<std::pair<std::string, std::string>, TimelineContact> contacts;
		for (const auto &contact : native_contacts) {
			const auto *first = &solved_->bodies.at(contact.first_instance_index);
			const auto *second = &solved_->bodies.at(contact.second_instance_index);
			if (second->body_id < first->body_id) {
				std::swap(first, second);
			}
			auto [entry, inserted] = contacts.try_emplace(
				std::make_pair(first->body_id, second->body_id), TimelineContact{*first, *second, contact.impulse});
			if (!inserted) {
				entry->second.peak_impulse = std::max(entry->second.peak_impulse, contact.impulse);
			}
		}
		std::vector<TimelineContact> ordered;
		ordered.reserve(contacts.size());
		for (auto &entry : contacts) {
			ordered.push_back(std::move(entry.second));
		}
		record_contacts_(solved_tick_, ordered);
	}
}

TimelineRenderSample RenderSceneTimeline::sample(double time_seconds)
{
	if (!physics_) {
		if (grammar_uses_time_ && time_seconds != 0.0) {
			sampled_ = std::make_unique<TimelineSceneState>(evaluate_grammar_(time_seconds));
			return {sampled_->scene(), sampled_->bodies};
		}
		return {solved_->scene(), solved_->bodies};
	}

	double requested_tick = time_seconds * kSceneSimulationTicksPerSecond;
	const double nearest_tick = std::round(requested_tick);
	const double tolerance = 4.0 * std::numeric_limits<double>::epsilon() *
	                         std::max(1.0, std::abs(requested_tick));
	if (std::abs(requested_tick - nearest_tick) <= tolerance) {
		requested_tick = nearest_tick;
	}
	const auto lower_tick = static_cast<std::uint64_t>(std::floor(requested_tick));
	const auto upper_tick = static_cast<std::uint64_t>(std::ceil(requested_tick));
	if (upper_tick < solved_tick_) {
		throw std::logic_error("Timeline samples must be evaluated in increasing time order.");
	}
	while (solved_tick_ < upper_tick) {
		step();
	}
	if (lower_tick == upper_tick) {
		return {solved_->scene(), solved_->bodies, solved_tick_};
	}

	// Evaluate kinematic geometry at the exact sample. Interpolate render
	// poses in a separate context so subframes never affect the solver.
	if (!sampled_ || grammar_uses_time_) {
		sampled_ = std::make_unique<TimelineSceneState>(evaluate_grammar_(time_seconds));
	}
	const double fraction = requested_tick - static_cast<double>(lower_tick);
	auto &instances = sampled_->scene().primitive_instances;
	for (std::size_t index = 0; index < instances.size(); ++index) {
		if (instances[index].immovable) {
			continue;
		}
		const auto &id = sampled_->bodies[index].body_id;
		if (const auto *lower = previous_->find(id)) {
			const PrimitiveInstance *upper = solved_->find(id);
			const BodyPose pose = upper != nullptr && !upper->immovable
				? lower->interpolate(BodyPose::capture(*upper), static_cast<float>(fraction)) : *lower;
			pose.apply(instances[index]);
		}
	}
	sampled_->scene().time = static_cast<float>(time_seconds);
	return {sampled_->scene(), sampled_->bodies, lower_tick, fraction};
}
