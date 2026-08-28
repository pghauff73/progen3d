#pragma once

#include "spatial/model/SpatialObjectId.h"

#include <string>
#include <utility>
#include <vector>

enum class BuildingModelValidationCode {
	SafetyCeilingExceeded,
	InvalidIdentifier,
	DuplicateConceptId,
	DuplicateConceptName,
	UndefinedBroaderConcept,
	ConceptHierarchyCycle,
	DuplicateRoleId,
	DuplicateRoleName,
	UndefinedRole,
	DuplicateRoleAssignment,
	UndefinedProfileObject,
	DuplicateObjectProfile,
	MissingObjectProfile,
	UndefinedProfileConcept,
	IncompleteApplicability,
	GeometryApplicabilityMismatch,
	DuplicateFunctionId,
	UndefinedFunction,
	DuplicateFunctionAllocationId,
	UndefinedFunctionAllocationObject,
	FunctionalDependencyCycle,
	UncoveredHardFunction,
	DuplicateServiceSystemId,
	UndefinedServiceSystemOwner,
	DuplicateServicePortId,
	UndefinedServicePortOwner,
	UndefinedServicePortSystem,
	UnsupportedServiceMedium,
	DuplicateServiceFlowId,
	UndefinedServiceFlowEndpoint,
	InvalidServiceFlowDirection,
	ServiceMediumMismatch,
	OrphanRequiredServicePort,
	ServiceFlowCycle,
	DuplicateRelationshipAssertion,
	DuplicateCanonicalRelationshipFact,
	UndefinedCanonicalRelationshipFact,
	CanonicalRelationshipEndpointMismatch,
	DuplicateRequirementId,
	UndefinedRequirementTarget,
	UndefinedRequirementDependency,
	RequirementDependencyCycle,
	MissingRequirementEvaluation,
	DuplicateRequirementEvaluation,
	FailedHardRequirement,
	UnknownHardRequirement,
	InvalidRequirementEvidence,
	RequirementEvaluationContradictsModel,
	PassedRequirementDependsOnUnresolvedRequirement,
	DuplicateEvidenceId,
	UndefinedEvidenceReference,
	InvalidEvidenceHash,
	InvalidStateTransition,
	InvalidScenario,
	AggregateHashMissing
};

class BuildingModelValidationIssue {
public:
	BuildingModelValidationIssue(BuildingModelValidationCode code,
	                             std::string message,
	                             std::vector<SpatialObjectId> object_path = {})
		: code_(code),
		  message_(std::move(message)),
		  object_path_(std::move(object_path)) {}

	BuildingModelValidationCode code() const { return code_; }
	const std::string &message() const { return message_; }
	const std::vector<SpatialObjectId> &objectPath() const { return object_path_; }

private:
	BuildingModelValidationCode code_ = BuildingModelValidationCode::InvalidIdentifier;
	std::string message_;
	std::vector<SpatialObjectId> object_path_;
};
