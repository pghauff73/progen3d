#pragma once

#include "building/model/BuildingRequirement.h"

class IdentityRequirement : public BuildingRequirement {
public:
	IdentityRequirement(BuildingRequirementId requirement_id,
	                    BuildingRequirementCriticality criticality,
	                    BuildingRequirementTarget target,
	                    std::vector<BuildingRequirementId> dependency_ids = {})
		: BuildingRequirement(
			std::move(requirement_id), BuildingRequirementKind::Identity, criticality,
			std::move(target), std::move(dependency_ids)) {}
};
