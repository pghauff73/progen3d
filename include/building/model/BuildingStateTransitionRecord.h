#pragma once

#include "building/model/BuildingEvidenceReference.h"
#include "building/model/BuildingScenarioId.h"
#include "spatial/model/SpatialObjectId.h"

#include <string>
#include <utility>

class BuildingStateTransitionRecord {
public:
	BuildingStateTransitionRecord(BuildingScenarioId scenario_id,
	                              SpatialObjectId object_id,
	                              std::string state_facet,
	                              std::string previous_state,
	                              std::string requested_state,
	                              BuildingEvidenceReference evidence)
		: scenario_id_(std::move(scenario_id)),
		  object_id_(std::move(object_id)),
		  state_facet_(std::move(state_facet)),
		  previous_state_(std::move(previous_state)),
		  requested_state_(std::move(requested_state)),
		  evidence_(std::move(evidence)) {}

	const BuildingScenarioId &scenarioId() const { return scenario_id_; }
	const SpatialObjectId &objectId() const { return object_id_; }
	const std::string &stateFacet() const { return state_facet_; }
	const std::string &previousState() const { return previous_state_; }
	const std::string &requestedState() const { return requested_state_; }
	const BuildingEvidenceReference &evidence() const { return evidence_; }

private:
	BuildingScenarioId scenario_id_;
	SpatialObjectId object_id_;
	std::string state_facet_;
	std::string previous_state_;
	std::string requested_state_;
	BuildingEvidenceReference evidence_;
};
