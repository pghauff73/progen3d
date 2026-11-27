#include "audio/service/SceneAudioEventEvaluator.h"

#include "Context.h"
#include "spatial/model/SpatialBuildingModel.h"

#include <algorithm>
#include <glm/geometric.hpp>

namespace {

constexpr double collision_cooldown_seconds = 0.35;

std::vector<std::size_t> lookupInstances(const SceneGenerationContext &scene,
                                         const std::string &object_id)
{
	const auto &model = scene.spatialBuildingModel();
	if (model == nullptr || object_id.empty()) return {};
	std::vector<std::size_t> instances;
	for (const auto &binding : model->bindingsFor(SpatialObjectId(object_id))) {
		instances.push_back(binding.primitiveInstanceIndex());
	}
	return instances;
}

std::optional<glm::vec3> objectPosition(const SceneGenerationContext &scene,
                                       const std::string &object_id,
                                       const std::vector<std::size_t> &instances)
{
	if (instances.empty()) {
		const auto &model = scene.spatialBuildingModel();
		const auto *object = model != nullptr ? model->objects().find(SpatialObjectId(object_id)) : nullptr;
		if (object == nullptr) return std::nullopt;
		return glm::vec3(object->frameState().resolvedWorldTransform()[3]);
	}
	glm::dvec3 sum(0.0);
	std::size_t count = 0;
	for (std::size_t index : instances) {
		const ScenePrimitiveInstance &instance = scene.primitive_instances[index];
		if (instance.removed) continue;
		sum += glm::dvec3(scene.getInstanceCenter(instance));
		++count;
	}
	if (count == 0) return std::nullopt;
	return glm::vec3(sum / static_cast<double>(count));
}

bool matchesCollision(const SimulationContact &contact,
                      const std::vector<std::size_t> &first,
                      const std::vector<std::size_t> &second)
{
	const auto contains = [](const std::vector<std::size_t> &indices, std::size_t index) {
		return std::find(indices.begin(), indices.end(), index) != indices.end();
	};
	return (contains(first, contact.first_instance_index) &&
	        contains(second, contact.second_instance_index)) ||
	       (contains(first, contact.second_instance_index) &&
	        contains(second, contact.first_instance_index));
}

} // namespace

void SceneAudioEventEvaluator::replaceProgram(const std::vector<AudioEventDefinition> &definitions,
                                               const SceneGenerationContext &scene,
                                               double seconds)
{
	definitions_ = definitions;
	states_.assign(definitions.size(), TriggerState{});
	has_collision_triggers_ = std::any_of(definitions.begin(), definitions.end(),
		[](const AudioEventDefinition &definition) {
			return std::holds_alternative<CollisionAudioTrigger>(definition.trigger);
		});
	bindScene(scene);
	rebase(scene, seconds);
}

void SceneAudioEventEvaluator::bindScene(const SceneGenerationContext &scene)
{
	for (std::size_t index = 0; index < definitions_.size(); ++index) {
		const AudioEventTrigger &trigger = definitions_[index].trigger;
		TriggerState &state = states_[index];
		if (const auto *simulation = std::get_if<SimulationAudioTrigger>(&trigger)) {
			state.object_instances = lookupInstances(scene, simulation->object_id);
		} else if (const auto *collision = std::get_if<CollisionAudioTrigger>(&trigger)) {
			state.object_instances = lookupInstances(scene, collision->first_object_id);
			state.other_instances = lookupInstances(scene, collision->second_object_id);
		} else if (const auto *region = std::get_if<RegionAudioTrigger>(&trigger)) {
			state.object_instances = lookupInstances(scene, region->object_id);
		}
	}
}

void SceneAudioEventEvaluator::rebase(const SceneGenerationContext &scene, double seconds)
{
	previous_seconds_ = seconds;
	run_started_ = false;
	for (std::size_t index = 0; index < definitions_.size(); ++index) {
		const AudioEventTrigger &trigger = definitions_[index].trigger;
		TriggerState &state = states_[index];
		state.inside_region.reset();
		state.last_collision_seconds.reset();
		state.collision_active = false;
		state.motion = objectMotion(scene, state.object_instances);
		const auto *region = std::get_if<RegionAudioTrigger>(&trigger);
		if (region != nullptr && !region->object_id.empty()) {
			observeRegion(*region, objectPosition(scene, region->object_id, state.object_instances),
			              state.inside_region);
		}
	}
}

std::optional<SceneAudioEventEvaluator::ObjectMotion> SceneAudioEventEvaluator::objectMotion(
	const SceneGenerationContext &scene, const std::vector<std::size_t> &instances)
{
	if (instances.empty()) return std::nullopt;
	ObjectMotion motion = ObjectMotion::Fixed;
	for (std::size_t index : instances) {
		const ScenePrimitiveInstance &instance = scene.primitive_instances[index];
		if (instance.immovable) continue;
		if (instance.removed) {
			if (motion == ObjectMotion::Fixed) motion = ObjectMotion::Removed;
		} else if (!instance.simulation_active) {
			if (motion != ObjectMotion::Sleeping) motion = ObjectMotion::Stopped;
		} else if (instance.sleeping) {
			motion = ObjectMotion::Sleeping;
		} else {
			return ObjectMotion::Moving;
		}
	}
	return motion;
}

bool SceneAudioEventEvaluator::observeRegion(const RegionAudioTrigger &region,
                                              const std::optional<glm::vec3> &position,
                                              std::optional<bool> &inside)
{
	const auto was_inside = inside;
	if (!position) {
		inside.reset();
		return false;
	}
	const glm::dvec3 offset = glm::dvec3(*position) - glm::dvec3(region.center);
	inside = glm::dot(offset, offset) <= static_cast<double>(region.radius) * region.radius;
	return was_inside.has_value() && was_inside != inside &&
	       *inside == (region.transition == AudioRegionTransition::Enter);
}

DueAudioEvents SceneAudioEventEvaluator::advance(const SceneGenerationContext &scene,
                                                 double seconds,
                                                 const std::vector<SimulationContact> *physics_contacts)
{
	DueAudioEvents due;
	if (seconds <= previous_seconds_) return due;
	for (std::size_t index = 0; index < definitions_.size(); ++index) {
		const AudioEventDefinition &definition = definitions_[index];
		TriggerState &state = states_[index];
		bool triggered = false;
		if (const auto *time = std::get_if<TimeAudioTrigger>(&definition.trigger)) {
			triggered = time->seconds <= seconds &&
				(time->seconds > previous_seconds_ ||
				 (!run_started_ && time->seconds == 0.0 && previous_seconds_ == 0.0));
		} else if (const auto *simulation = std::get_if<SimulationAudioTrigger>(&definition.trigger)) {
			if (simulation->event == SimulationAudioEvent::Start) {
				triggered = !run_started_;
			} else if (physics_contacts != nullptr) {
				const auto previous_motion = state.motion;
				state.motion = objectMotion(scene, state.object_instances);
				if (previous_motion && state.motion && previous_motion != state.motion) {
					switch (simulation->event) {
					case SimulationAudioEvent::Start: break;
					case SimulationAudioEvent::Sleep: triggered = state.motion == ObjectMotion::Sleeping; break;
					case SimulationAudioEvent::Wake: triggered = previous_motion == ObjectMotion::Sleeping &&
					                                              state.motion == ObjectMotion::Moving; break;
					case SimulationAudioEvent::Stop: triggered = state.motion == ObjectMotion::Stopped; break;
					case SimulationAudioEvent::Remove: triggered = state.motion == ObjectMotion::Removed; break;
					}
				}
			}
		} else if (const auto *collision = std::get_if<CollisionAudioTrigger>(&definition.trigger)) {
			if (physics_contacts == nullptr) continue;
			const bool active = std::any_of(physics_contacts->begin(), physics_contacts->end(),
				[&](const SimulationContact &contact) {
					return contact.impulse >= collision->minimum_impulse &&
					       matchesCollision(contact, state.object_instances, state.other_instances);
				});
			triggered = active && !state.collision_active &&
				(!state.last_collision_seconds ||
				 seconds - *state.last_collision_seconds >= collision_cooldown_seconds);
			if (triggered) state.last_collision_seconds = seconds;
			state.collision_active = active;
		} else if (const auto *region = std::get_if<RegionAudioTrigger>(&definition.trigger)) {
			if (region->object_id.empty()) continue;
			triggered = observeRegion(*region, objectPosition(scene, region->object_id, state.object_instances),
			                          state.inside_region);
		}
		if (triggered) due.push_back(&definition);
	}
	const auto eventTime = [&](const AudioEventDefinition *definition) {
		if (const auto *time = std::get_if<TimeAudioTrigger>(&definition->trigger)) return time->seconds;
		const auto *simulation = std::get_if<SimulationAudioTrigger>(&definition->trigger);
		return simulation != nullptr && simulation->event == SimulationAudioEvent::Start
			? previous_seconds_ : seconds;
	};
	std::stable_sort(due.begin(), due.end(), [&](const auto *first, const auto *second) {
		return eventTime(first) < eventTime(second);
	});
	previous_seconds_ = seconds;
	run_started_ = true;
	return due;
}

DueAudioEvents SceneAudioEventEvaluator::observeCamera(const glm::vec3 &position, bool rebase_camera)
{
	DueAudioEvents due;
	for (std::size_t index = 0; index < definitions_.size(); ++index) {
		const AudioEventDefinition &definition = definitions_[index];
		const auto *region = std::get_if<RegionAudioTrigger>(&definition.trigger);
		if (region == nullptr || !region->object_id.empty()) continue;
		auto &inside = states_[index].inside_region;
		if (rebase_camera) inside.reset();
		if (observeRegion(*region, position, inside)) due.push_back(&definition);
	}
	return due;
}
