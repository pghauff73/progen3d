#pragma once

#include "building/model/BuildingRequirement.h"
#include "spatial/model/SpatialObjectId.h"

class ContainmentRequirement : public BuildingRequirement {
public:
	ContainmentRequirement(BuildingRequirementId requirement_id,
	                       BuildingRequirementCriticality criticality,
	                       BuildingRequirementTarget target,
	                       SpatialObjectId required_container_id,
	                       std::vector<BuildingRequirementId> dependency_ids = {})
		: BuildingRequirement(
			std::move(requirement_id), BuildingRequirementKind::Containment, criticality,
			std::move(target), std::move(dependency_ids)),
		  required_container_id_(std::move(required_container_id)) {}

	const SpatialObjectId &requiredContainerId() const { return required_container_id_; }

private:
	SpatialObjectId required_container_id_;
};
