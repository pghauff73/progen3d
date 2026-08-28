#pragma once

#include "building/model/BuildingRequirementCriticality.h"
#include "building/model/BuildingRequirementId.h"
#include "building/model/BuildingRequirementKind.h"
#include "building/model/BuildingRequirementTarget.h"

#include <utility>
#include <vector>

class BuildingRequirement {
public:
	virtual ~BuildingRequirement() = default;

	const BuildingRequirementId &requirementId() const { return requirement_id_; }
	BuildingRequirementKind kind() const { return kind_; }
	BuildingRequirementCriticality criticality() const { return criticality_; }
	const BuildingRequirementTarget &target() const { return target_; }
	const std::vector<BuildingRequirementId> &dependencyIds() const
	{
		return dependency_ids_;
	}

protected:
	BuildingRequirement(BuildingRequirementId requirement_id,
	                    BuildingRequirementKind kind,
	                    BuildingRequirementCriticality criticality,
	                    BuildingRequirementTarget target,
	                    std::vector<BuildingRequirementId> dependency_ids)
		: requirement_id_(std::move(requirement_id)),
		  kind_(kind),
		  criticality_(criticality),
		  target_(std::move(target)),
		  dependency_ids_(std::move(dependency_ids)) {}

private:
	BuildingRequirementId requirement_id_;
	BuildingRequirementKind kind_ = BuildingRequirementKind::Identity;
	BuildingRequirementCriticality criticality_ = BuildingRequirementCriticality::Informational;
	BuildingRequirementTarget target_ = BuildingRequirementTarget::wholeBuilding();
	std::vector<BuildingRequirementId> dependency_ids_;
};
