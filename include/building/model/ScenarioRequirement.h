#pragma once

#include "building/model/BuildingRequirement.h"
#include "building/model/BuildingScenarioId.h"

class ScenarioRequirement : public BuildingRequirement {
public:
	ScenarioRequirement(BuildingRequirementId requirement_id,
	                    BuildingRequirementCriticality criticality,
	                    BuildingRequirementTarget target,
	                    BuildingScenarioId scenario_id,
	                    std::vector<BuildingRequirementId> dependency_ids = {})
		: BuildingRequirement(
			std::move(requirement_id), BuildingRequirementKind::Scenario, criticality,
			std::move(target), std::move(dependency_ids)),
		  scenario_id_(std::move(scenario_id)) {}

	const BuildingScenarioId &scenarioId() const { return scenario_id_; }

private:
	BuildingScenarioId scenario_id_;
};
