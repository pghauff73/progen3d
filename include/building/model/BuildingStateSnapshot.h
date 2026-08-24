#pragma once

#include "building/model/BuildingObjectStateRecord.h"
#include "building/model/BuildingScenarioId.h"

#include <cstdint>
#include <utility>
#include <vector>

class BuildingStateSnapshot {
public:
	BuildingStateSnapshot(BuildingScenarioId scenario_id,
	                      std::vector<BuildingObjectStateRecord> object_states,
	                      std::uint64_t snapshot_hash)
		: scenario_id_(std::move(scenario_id)),
		  object_states_(std::move(object_states)),
		  snapshot_hash_(snapshot_hash) {}

	const BuildingScenarioId &scenarioId() const { return scenario_id_; }
	const std::vector<BuildingObjectStateRecord> &objectStates() const
	{
		return object_states_;
	}
	std::uint64_t snapshotHash() const { return snapshot_hash_; }

private:
	BuildingScenarioId scenario_id_;
	std::vector<BuildingObjectStateRecord> object_states_;
	std::uint64_t snapshot_hash_ = 0;
};
