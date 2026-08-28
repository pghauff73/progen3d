#pragma once

#include "building/model/BuildingConceptId.h"
#include "building/model/BuildingEvidenceReference.h"
#include "building/model/BuildingFunctionAllocationId.h"
#include "building/model/BuildingObjectModelApplicability.h"
#include "building/model/BuildingRequirementId.h"
#include "building/model/BuildingRoleId.h"
#include "building/model/BuildingScenarioId.h"
#include "building/model/BuildingServicePortId.h"
#include "spatial/model/SpatialObjectId.h"

#include <cstdint>
#include <utility>
#include <vector>

class BuildingObjectSemanticProfile {
public:
	BuildingObjectSemanticProfile(
		SpatialObjectId object_id,
		BuildingConceptId concept_id,
		std::vector<BuildingRoleId> role_ids,
		std::vector<BuildingFunctionAllocationId> function_allocation_ids,
		std::vector<BuildingServicePortId> service_port_ids,
		std::vector<BuildingRequirementId> requirement_ids,
		std::vector<BuildingScenarioId> scenario_ids,
		BuildingObjectModelApplicability applicability,
		std::vector<BuildingEvidenceReference> evidence_references,
		std::uint64_t profile_revision,
		std::uint64_t profile_hash)
		: object_id_(std::move(object_id)),
		  concept_id_(std::move(concept_id)),
		  role_ids_(std::move(role_ids)),
		  function_allocation_ids_(std::move(function_allocation_ids)),
		  service_port_ids_(std::move(service_port_ids)),
		  requirement_ids_(std::move(requirement_ids)),
		  scenario_ids_(std::move(scenario_ids)),
		  applicability_(applicability),
		  evidence_references_(std::move(evidence_references)),
		  profile_revision_(profile_revision),
		  profile_hash_(profile_hash) {}

	const SpatialObjectId &objectId() const { return object_id_; }
	const BuildingConceptId &conceptId() const { return concept_id_; }
	const std::vector<BuildingRoleId> &roleIds() const { return role_ids_; }
	const std::vector<BuildingFunctionAllocationId> &functionAllocationIds() const
	{
		return function_allocation_ids_;
	}
	const std::vector<BuildingServicePortId> &servicePortIds() const
	{
		return service_port_ids_;
	}
	const std::vector<BuildingRequirementId> &requirementIds() const
	{
		return requirement_ids_;
	}
	const std::vector<BuildingScenarioId> &scenarioIds() const { return scenario_ids_; }
	const BuildingObjectModelApplicability &applicability() const { return applicability_; }
	const std::vector<BuildingEvidenceReference> &evidenceReferences() const
	{
		return evidence_references_;
	}
	std::uint64_t profileRevision() const { return profile_revision_; }
	std::uint64_t profileHash() const { return profile_hash_; }

	BuildingObjectSemanticProfile withProfileHash(std::uint64_t profile_hash) const
	{
		return BuildingObjectSemanticProfile(
			object_id_, concept_id_, role_ids_, function_allocation_ids_, service_port_ids_,
			requirement_ids_, scenario_ids_, applicability_, evidence_references_,
			profile_revision_, profile_hash);
	}

private:
	SpatialObjectId object_id_;
	BuildingConceptId concept_id_;
	std::vector<BuildingRoleId> role_ids_;
	std::vector<BuildingFunctionAllocationId> function_allocation_ids_;
	std::vector<BuildingServicePortId> service_port_ids_;
	std::vector<BuildingRequirementId> requirement_ids_;
	std::vector<BuildingScenarioId> scenario_ids_;
	BuildingObjectModelApplicability applicability_{
		BuildingModelApplicability::NotApplicable,
		BuildingModelApplicability::NotApplicable,
		BuildingModelApplicability::NotApplicable,
		BuildingModelApplicability::Applicable,
		BuildingModelApplicability::NotApplicable,
		BuildingModelApplicability::NotApplicable,
		BuildingModelApplicability::Applicable,
		BuildingModelApplicability::NotApplicable,
		BuildingModelApplicability::NotApplicable,
		BuildingModelApplicability::NotApplicable};
	std::vector<BuildingEvidenceReference> evidence_references_;
	std::uint64_t profile_revision_ = 0;
	std::uint64_t profile_hash_ = 0;
};
