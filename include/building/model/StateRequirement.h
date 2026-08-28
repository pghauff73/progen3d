#pragma once

#include "building/model/BuildingRequirement.h"

#include <string>

class StateRequirement : public BuildingRequirement {
public:
	StateRequirement(BuildingRequirementId requirement_id,
	                 BuildingRequirementCriticality criticality,
	                 BuildingRequirementTarget target,
	                 std::string required_state,
	                 std::vector<BuildingRequirementId> dependency_ids = {})
		: BuildingRequirement(
			std::move(requirement_id), BuildingRequirementKind::State, criticality,
			std::move(target), std::move(dependency_ids)),
		  required_state_(std::move(required_state)) {}

	const std::string &requiredState() const { return required_state_; }

private:
	std::string required_state_;
};
