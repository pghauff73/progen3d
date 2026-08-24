#pragma once

#include "building/model/BuildingScenarioId.h"

#include <string>
#include <utility>

class BuildingScenario {
public:
	BuildingScenario(BuildingScenarioId scenario_id,
	                 std::string canonical_name,
	                 std::string purpose)
		: scenario_id_(std::move(scenario_id)),
		  canonical_name_(std::move(canonical_name)),
		  purpose_(std::move(purpose)) {}

	const BuildingScenarioId &scenarioId() const { return scenario_id_; }
	const std::string &canonicalName() const { return canonical_name_; }
	const std::string &purpose() const { return purpose_; }

private:
	BuildingScenarioId scenario_id_;
	std::string canonical_name_;
	std::string purpose_;
};
