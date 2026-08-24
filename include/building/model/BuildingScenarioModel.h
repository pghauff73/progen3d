#pragma once

#include "building/model/BuildingScenario.h"
#include "building/model/BuildingStateSnapshot.h"
#include "building/model/BuildingStateTransitionRecord.h"
#include "building/relationship/BuildingScenarioParticipation.h"

#include <utility>
#include <vector>

class BuildingScenarioModel {
public:
	BuildingScenarioModel() = default;
	BuildingScenarioModel(std::vector<BuildingScenario> scenarios,
	                     std::vector<BuildingScenarioParticipation> participations,
	                     std::vector<BuildingStateSnapshot> snapshots,
	                     std::vector<BuildingStateTransitionRecord> transitions)
		: scenarios_(std::move(scenarios)),
		  participations_(std::move(participations)),
		  snapshots_(std::move(snapshots)),
		  transitions_(std::move(transitions)) {}

	const std::vector<BuildingScenario> &scenarios() const { return scenarios_; }
	const std::vector<BuildingScenarioParticipation> &participations() const
	{
		return participations_;
	}
	const std::vector<BuildingStateSnapshot> &snapshots() const { return snapshots_; }
	const std::vector<BuildingStateTransitionRecord> &transitions() const
	{
		return transitions_;
	}

private:
	std::vector<BuildingScenario> scenarios_;
	std::vector<BuildingScenarioParticipation> participations_;
	std::vector<BuildingStateSnapshot> snapshots_;
	std::vector<BuildingStateTransitionRecord> transitions_;
};
