#pragma once

#include "building/model/BuildingModelApplicability.h"
#include "building/model/BuildingScenarioId.h"
#include "spatial/model/SpatialObjectId.h"

#include <utility>

class BuildingScenarioParticipation {
public:
	BuildingScenarioParticipation(BuildingScenarioId scenario_id,
	                              SpatialObjectId object_id,
	                              BuildingModelApplicability applicability)
		: scenario_id_(std::move(scenario_id)),
		  object_id_(std::move(object_id)),
		  applicability_(applicability) {}

	const BuildingScenarioId &scenarioId() const { return scenario_id_; }
	const SpatialObjectId &objectId() const { return object_id_; }
	BuildingModelApplicability applicability() const { return applicability_; }

private:
	BuildingScenarioId scenario_id_;
	SpatialObjectId object_id_;
	BuildingModelApplicability applicability_ = BuildingModelApplicability::NotApplicable;
};
