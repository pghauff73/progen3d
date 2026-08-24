#pragma once

#include "building/model/BuildingConceptId.h"
#include "building/model/BuildingRequirement.h"

class ClassificationRequirement : public BuildingRequirement {
public:
	ClassificationRequirement(BuildingRequirementId requirement_id,
	                          BuildingRequirementCriticality criticality,
	                          BuildingRequirementTarget target,
	                          BuildingConceptId required_concept_id,
	                          std::vector<BuildingRequirementId> dependency_ids = {})
		: BuildingRequirement(
			std::move(requirement_id), BuildingRequirementKind::Classification, criticality,
			std::move(target), std::move(dependency_ids)),
		  required_concept_id_(std::move(required_concept_id)) {}

	const BuildingConceptId &requiredConceptId() const { return required_concept_id_; }

private:
	BuildingConceptId required_concept_id_;
};
