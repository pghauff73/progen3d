#pragma once

#include "building/model/BuildingRequirement.h"
#include "spatial/model/SpatialConnectionId.h"

class ConnectionRequirement : public BuildingRequirement {
public:
	ConnectionRequirement(BuildingRequirementId requirement_id,
	                      BuildingRequirementCriticality criticality,
	                      BuildingRequirementTarget target,
	                      SpatialConnectionId required_connection_id,
	                      std::vector<BuildingRequirementId> dependency_ids = {})
		: BuildingRequirement(
			std::move(requirement_id), BuildingRequirementKind::Connection, criticality,
			std::move(target), std::move(dependency_ids)),
		  required_connection_id_(std::move(required_connection_id)) {}

	const SpatialConnectionId &requiredConnectionId() const
	{
		return required_connection_id_;
	}

private:
	SpatialConnectionId required_connection_id_;
};
