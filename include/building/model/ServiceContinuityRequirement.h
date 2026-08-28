#pragma once

#include "building/model/BuildingRequirement.h"
#include "building/model/BuildingServicePortId.h"

class ServiceContinuityRequirement : public BuildingRequirement {
public:
	ServiceContinuityRequirement(BuildingRequirementId requirement_id,
	                             BuildingRequirementCriticality criticality,
	                             BuildingRequirementTarget target,
	                             BuildingServicePortId source_port_id,
	                             BuildingServicePortId target_port_id,
	                             std::vector<BuildingRequirementId> dependency_ids = {})
		: BuildingRequirement(
			std::move(requirement_id), BuildingRequirementKind::ServiceContinuity,
			criticality, std::move(target), std::move(dependency_ids)),
		  source_port_id_(std::move(source_port_id)),
		  target_port_id_(std::move(target_port_id)) {}

	const BuildingServicePortId &sourcePortId() const { return source_port_id_; }
	const BuildingServicePortId &targetPortId() const { return target_port_id_; }

private:
	BuildingServicePortId source_port_id_;
	BuildingServicePortId target_port_id_;
};
