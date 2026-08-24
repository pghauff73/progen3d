#pragma once

#include "building/model/BuildingRequirement.h"

class ClearanceRequirement : public BuildingRequirement {
public:
	ClearanceRequirement(BuildingRequirementId requirement_id,
	                     BuildingRequirementCriticality criticality,
	                     BuildingRequirementTarget target,
	                     float minimum_clearance,
	                     float maximum_clearance,
	                     float tolerance,
	                     std::vector<BuildingRequirementId> dependency_ids = {})
		: BuildingRequirement(
			std::move(requirement_id), BuildingRequirementKind::Clearance, criticality,
			std::move(target), std::move(dependency_ids)),
		  minimum_clearance_(minimum_clearance),
		  maximum_clearance_(maximum_clearance),
		  tolerance_(tolerance) {}

	float minimumClearance() const { return minimum_clearance_; }
	float maximumClearance() const { return maximum_clearance_; }
	float tolerance() const { return tolerance_; }

private:
	float minimum_clearance_ = 0.0f;
	float maximum_clearance_ = 0.0f;
	float tolerance_ = 0.0f;
};
