#pragma once

#include "building/model/BuildingRequirement.h"
#include "spatial/model/BoundaryRepresentation.h"

class BoundaryRequirement : public BuildingRequirement {
public:
	BoundaryRequirement(BuildingRequirementId requirement_id,
	                    BuildingRequirementCriticality criticality,
	                    BuildingRequirementTarget target,
	                    BoundaryRepresentationKind minimum_representation,
	                    std::vector<BuildingRequirementId> dependency_ids = {})
		: BuildingRequirement(
			std::move(requirement_id), BuildingRequirementKind::Boundary, criticality,
			std::move(target), std::move(dependency_ids)),
		  minimum_representation_(minimum_representation) {}

	BoundaryRepresentationKind minimumRepresentation() const
	{
		return minimum_representation_;
	}

private:
	BoundaryRepresentationKind minimum_representation_ =
		BoundaryRepresentationKind::AxisAlignedBounding;
};
