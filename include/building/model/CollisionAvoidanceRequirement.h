#pragma once

#include "building/model/BuildingRequirement.h"

class CollisionAvoidanceRequirement : public BuildingRequirement {
public:
	CollisionAvoidanceRequirement(BuildingRequirementId requirement_id,
	                              BuildingRequirementCriticality criticality,
	                              BuildingRequirementTarget target,
	                              float maximum_penetration,
	                              std::vector<BuildingRequirementId> dependency_ids = {})
		: BuildingRequirement(
			std::move(requirement_id), BuildingRequirementKind::CollisionAvoidance,
			criticality, std::move(target), std::move(dependency_ids)),
		  maximum_penetration_(maximum_penetration) {}

	float maximumPenetration() const { return maximum_penetration_; }

private:
	float maximum_penetration_ = 0.0f;
};
