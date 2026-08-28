#pragma once

#include "building/model/BuildingFunctionId.h"
#include "building/model/BuildingRequirement.h"

#include <cstddef>

class FunctionalCoverageRequirement : public BuildingRequirement {
public:
	FunctionalCoverageRequirement(BuildingRequirementId requirement_id,
	                              BuildingRequirementCriticality criticality,
	                              BuildingRequirementTarget target,
	                              BuildingFunctionId function_id,
	                              std::size_t minimum_allocation_count,
	                              std::vector<BuildingRequirementId> dependency_ids = {})
		: BuildingRequirement(
			std::move(requirement_id), BuildingRequirementKind::FunctionalCoverage,
			criticality, std::move(target), std::move(dependency_ids)),
		  function_id_(std::move(function_id)),
		  minimum_allocation_count_(minimum_allocation_count) {}

	const BuildingFunctionId &functionId() const { return function_id_; }
	std::size_t minimumAllocationCount() const { return minimum_allocation_count_; }

private:
	BuildingFunctionId function_id_;
	std::size_t minimum_allocation_count_ = 1;
};
