#include "building/service/BuildingRequirementEvaluationService.h"

#include "building/model/BuildingClassificationModel.h"
#include "building/model/BoundaryRequirement.h"
#include "building/model/ClassificationRequirement.h"
#include "building/model/ClearanceRequirement.h"
#include "building/model/CollisionAvoidanceRequirement.h"
#include "building/model/ConnectionRequirement.h"
#include "building/model/ContainmentRequirement.h"
#include "building/model/BuildingEvidenceLedger.h"
#include "building/model/BuildingFunctionModel.h"
#include "building/model/FunctionalCoverageRequirement.h"
#include "building/model/IdentityRequirement.h"
#include "building/model/BuildingScenarioModel.h"
#include "building/model/BuildingServiceModel.h"
#include "building/model/ScenarioRequirement.h"
#include "building/model/ServiceContinuityRequirement.h"
#include "building/model/StateRequirement.h"
#include "building/service/BuildingServiceTopologyValidationService.h"
#include "spatial/model/SpatialBuildingModel.h"

#include <cmath>
#include <map>
#include <optional>
#include <set>

namespace {

bool target_exists(
	const BuildingRequirementTarget &target,
	const SpatialBuildingModel &spatial_model,
	const BuildingFunctionModel &function_model,
	const BuildingServiceModel &service_model,
	const BuildingScenarioModel &scenario_model)
{
	switch (target.kind()) {
	case BuildingRequirementTargetKind::Object:
		return spatial_model.objects().find(SpatialObjectId(target.identifier())) != nullptr;
	case BuildingRequirementTargetKind::Connection:
		for (const SpatialConnection &connection :
		     spatial_model.connectionGraph().connections()) {
			if (connection.connectionId().value() == target.identifier()) return true;
		}
		return false;
	case BuildingRequirementTargetKind::Function:
		return function_model.findFunction(BuildingFunctionId(target.identifier())) != nullptr;
	case BuildingRequirementTargetKind::ServicePort:
		return service_model.findPort(BuildingServicePortId(target.identifier())) != nullptr;
	case BuildingRequirementTargetKind::Scenario:
		for (const BuildingScenario &scenario : scenario_model.scenarios()) {
			if (scenario.scenarioId().value() == target.identifier()) return true;
		}
		return false;
	case BuildingRequirementTargetKind::WholeBuilding:
		return true;
	}
	return false;
}

bool visit_requirement(
	const BuildingRequirementId &requirement_id,
	const std::map<BuildingRequirementId, const BuildingRequirement *> &requirements,
	std::set<BuildingRequirementId> *visiting,
	std::set<BuildingRequirementId> *visited)
{
	if (visited->find(requirement_id) != visited->end()) return true;
	if (!visiting->insert(requirement_id).second) return false;
	const auto found = requirements.find(requirement_id);
	if (found != requirements.end()) {
		for (const BuildingRequirementId &dependency_id : found->second->dependencyIds()) {
			if (!visit_requirement(dependency_id, requirements, visiting, visited)) return false;
		}
	}
	visiting->erase(requirement_id);
	visited->insert(requirement_id);
	return true;
}

std::optional<bool> evaluate_requirement_against_models(
	const BuildingRequirement &requirement,
	const BuildingRequirementEvaluationRecord &evaluation,
	const SpatialBuildingModel &spatial_model,
	const BuildingClassificationModel &classification_model,
	const BuildingFunctionModel &function_model,
	const BuildingServiceModel &service_model,
	const BuildingScenarioModel &scenario_model,
	const std::map<BuildingRequirementId, const BuildingRequirementEvaluationRecord *> &evaluations)
{
	if (requirement.target().kind() == BuildingRequirementTargetKind::WholeBuilding) {
		if (requirement.dependencyIds().empty()) return std::nullopt;
		for (const BuildingRequirementId &dependency_id : requirement.dependencyIds()) {
			const auto dependency = evaluations.find(dependency_id);
			if (dependency == evaluations.end() ||
			    dependency->second->status() != BuildingRequirementEvaluationStatus::Passed) {
				return false;
			}
		}
		return true;
	}

	switch (requirement.kind()) {
	case BuildingRequirementKind::Identity:
		return spatial_model.objects().find(
			SpatialObjectId(requirement.target().identifier())) != nullptr;
	case BuildingRequirementKind::Classification: {
		const auto *classification_requirement =
			dynamic_cast<const ClassificationRequirement *>(&requirement);
		const BuildingObjectSemanticProfile *profile = classification_model.findProfile(
			SpatialObjectId(requirement.target().identifier()));
		return classification_requirement != nullptr && profile != nullptr &&
		       profile->conceptId() == classification_requirement->requiredConceptId();
	}
	case BuildingRequirementKind::Containment: {
		const auto *containment_requirement =
			dynamic_cast<const ContainmentRequirement *>(&requirement);
		const SpatialObjectId target_object_id(requirement.target().identifier());
		const SpatialObjectId *parent =
			spatial_model.containmentTree().parentOf(target_object_id);
		return containment_requirement != nullptr && parent != nullptr &&
		       *parent == containment_requirement->requiredContainerId();
	}
	case BuildingRequirementKind::Connection: {
		const auto *connection_requirement =
			dynamic_cast<const ConnectionRequirement *>(&requirement);
		if (connection_requirement == nullptr) return false;
		for (const SpatialConnection &connection :
		     spatial_model.connectionGraph().connections()) {
			if (connection.connectionId() ==
			    connection_requirement->requiredConnectionId()) return true;
		}
		return false;
	}
	case BuildingRequirementKind::Clearance: {
		const auto *clearance_requirement =
			dynamic_cast<const ClearanceRequirement *>(&requirement);
		if (clearance_requirement == nullptr ||
		    !evaluation.measuredValue().numericValue().has_value()) return false;
		const double measured = *evaluation.measuredValue().numericValue();
		return measured + clearance_requirement->tolerance() >=
			       clearance_requirement->minimumClearance() &&
		       measured - clearance_requirement->tolerance() <=
			       clearance_requirement->maximumClearance();
	}
	case BuildingRequirementKind::CollisionAvoidance: {
		const auto *collision_requirement =
			dynamic_cast<const CollisionAvoidanceRequirement *>(&requirement);
		return collision_requirement != nullptr &&
		       evaluation.measuredValue().numericValue().has_value() &&
		       *evaluation.measuredValue().numericValue() <=
			       collision_requirement->maximumPenetration() + evaluation.tolerance();
	}
	case BuildingRequirementKind::FunctionalCoverage: {
		const auto *coverage_requirement =
			dynamic_cast<const FunctionalCoverageRequirement *>(&requirement);
		if (coverage_requirement == nullptr) return false;
		std::size_t allocation_count = 0;
		for (const BuildingFunctionAllocationRelationship &allocation :
		     function_model.allocations()) {
			if (allocation.functionId() == coverage_requirement->functionId()) {
				++allocation_count;
			}
		}
		return allocation_count >= coverage_requirement->minimumAllocationCount();
	}
	case BuildingRequirementKind::ServiceContinuity: {
		const auto *continuity_requirement =
			dynamic_cast<const ServiceContinuityRequirement *>(&requirement);
		return continuity_requirement != nullptr &&
		       BuildingServiceTopologyValidationService()
			       .evaluateContinuity(
				       service_model, continuity_requirement->sourcePortId(),
				       continuity_requirement->targetPortId())
			       .reachable();
	}
	case BuildingRequirementKind::Boundary: {
		const auto *boundary_requirement =
			dynamic_cast<const BoundaryRequirement *>(&requirement);
		const SpatialObjectId target_object_id(requirement.target().identifier());
		const SpatialBuildingObject *object =
			spatial_model.objects().find(target_object_id);
		if (boundary_requirement == nullptr || object == nullptr) return false;
		for (const auto &representation : object->boundaryModel().representations()) {
			if (representation &&
			    representation->kind() == boundary_requirement->minimumRepresentation()) {
				return true;
			}
		}
		return boundary_requirement->minimumRepresentation() ==
			       BoundaryRepresentationKind::AxisAlignedBounding &&
		       !spatial_model.bindingsFor(target_object_id).empty();
	}
	case BuildingRequirementKind::Scenario: {
		const auto *scenario_requirement =
			dynamic_cast<const ScenarioRequirement *>(&requirement);
		if (scenario_requirement == nullptr) return false;
		for (const BuildingScenario &scenario : scenario_model.scenarios()) {
			if (scenario.scenarioId() == scenario_requirement->scenarioId()) return true;
		}
		return false;
	}
	case BuildingRequirementKind::State:
	{
		const auto *state_requirement =
			dynamic_cast<const StateRequirement *>(&requirement);
		if (state_requirement == nullptr) return false;
		for (const BuildingStateSnapshot &snapshot : scenario_model.snapshots()) {
			if (snapshot.scenarioId() != BuildingScenarioId("Scenario.ResolvedPlacement")) {
				continue;
			}
			for (const BuildingObjectStateRecord &state : snapshot.objectStates()) {
				if (state.objectId().value() != requirement.target().identifier()) continue;
				if (state_requirement->requiredState() == "Authored") {
					return state.placementState() == BuildingPlacementState::Authored;
				}
				if (state_requirement->requiredState() == "Resolved") {
					return state.placementState() == BuildingPlacementState::Resolved;
				}
				if (state_requirement->requiredState() == "ContactResolved") {
					return state.placementState() == BuildingPlacementState::ContactResolved;
				}
				return false;
			}
		}
		return false;
	}
	}
	return std::nullopt;
}

bool dependency_depth_exceeds(
	const BuildingRequirementId &requirement_id,
	const std::map<BuildingRequirementId, const BuildingRequirement *> &requirements,
	std::size_t current_depth,
	std::size_t maximum_depth,
	std::set<BuildingRequirementId> *path)
{
	if (current_depth > maximum_depth) return true;
	if (!path->insert(requirement_id).second) return false;
	const auto found = requirements.find(requirement_id);
	if (found != requirements.end()) {
		for (const BuildingRequirementId &dependency_id : found->second->dependencyIds()) {
			if (dependency_depth_exceeds(
					dependency_id, requirements, current_depth + 1, maximum_depth, path)) {
				path->erase(requirement_id);
				return true;
			}
		}
	}
	path->erase(requirement_id);
	return false;
}

} // namespace

BuildingModelValidationReport BuildingRequirementEvaluationService::validateFinalEvaluations(
	const BuildingRequirementModel &requirement_model,
	const SpatialBuildingModel &spatial_model,
	const BuildingClassificationModel &classification_model,
	const BuildingFunctionModel &function_model,
	const BuildingServiceModel &service_model,
	const BuildingScenarioModel &scenario_model,
	const BuildingEvidenceLedger &evidence_ledger,
	const BuildingModelSafetyLimits &limits) const
{
	BuildingModelValidationReport report;
	if (requirement_model.requirements().size() > limits.maximum_requirements) {
		report.addIssue(BuildingModelValidationIssue(
			BuildingModelValidationCode::SafetyCeilingExceeded,
			"Building requirement model exceeds the configured safety ceiling."));
	}
	std::map<BuildingRequirementId, const BuildingRequirement *> requirements;
	for (const auto &requirement : requirement_model.requirements()) {
		if (!requirement || requirement->requirementId().empty()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::InvalidIdentifier,
				"Building requirement collection contains a null or invalid requirement."));
			continue;
		}
		if (!requirements.emplace(requirement->requirementId(), requirement.get()).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::DuplicateRequirementId,
				"Duplicate building requirement ID '" +
					requirement->requirementId().value() + "'."));
		}
		if (!target_exists(
				requirement->target(), spatial_model, function_model, service_model,
				scenario_model)) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::UndefinedRequirementTarget,
				"Building requirement references an undefined target."));
		}
	}
	for (const auto &[requirement_id, requirement] : requirements) {
		for (const BuildingRequirementId &dependency_id : requirement->dependencyIds()) {
			if (requirements.find(dependency_id) == requirements.end()) {
				report.addIssue(BuildingModelValidationIssue(
					BuildingModelValidationCode::UndefinedRequirementDependency,
					"Building requirement '" + requirement_id.value() +
						"' references an undefined dependency."));
			}
		}
	}
	std::set<BuildingRequirementId> visiting;
	std::set<BuildingRequirementId> visited;
	for (const auto &[requirement_id, requirement] : requirements) {
		(void)requirement;
		if (!visit_requirement(requirement_id, requirements, &visiting, &visited)) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::RequirementDependencyCycle,
				"Building requirement dependency graph contains a cycle."));
			break;
		}
	}
	for (const auto &[requirement_id, requirement] : requirements) {
		(void)requirement;
		std::set<BuildingRequirementId> dependency_path;
		if (dependency_depth_exceeds(
				requirement_id, requirements, 0,
				limits.maximum_requirement_dependency_depth, &dependency_path)) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::SafetyCeilingExceeded,
				"Building requirement dependency depth exceeds the configured safety ceiling."));
			break;
		}
	}

	std::map<BuildingRequirementId, const BuildingRequirementEvaluationRecord *> evaluations;
	for (const BuildingRequirementEvaluationRecord &evaluation :
	     requirement_model.evaluationRecords()) {
		if (!evaluations.emplace(evaluation.requirementId(), &evaluation).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::DuplicateRequirementEvaluation,
				"Duplicate evaluation for requirement '" +
					evaluation.requirementId().value() + "'."));
		}
		if (!std::isfinite(evaluation.tolerance()) || evaluation.tolerance() < 0.0 ||
		    (evaluation.measuredValue().numericValue().has_value() &&
		     !std::isfinite(*evaluation.measuredValue().numericValue())) ||
		    (evaluation.requiredValue().numericValue().has_value() &&
		     !std::isfinite(*evaluation.requiredValue().numericValue())) ||
		    evaluation.evidenceHash() == 0 || evaluation.evidenceReferences().empty()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::InvalidRequirementEvidence,
				"Requirement evaluation has invalid tolerance, evidence references, or hash."));
		}
		for (const BuildingEvidenceReference &reference : evaluation.evidenceReferences()) {
			if (evidence_ledger.find(reference.evidenceId()) == nullptr) {
				report.addIssue(BuildingModelValidationIssue(
					BuildingModelValidationCode::UndefinedEvidenceReference,
					"Requirement evaluation references undefined evidence '" +
						reference.evidenceId().value() + "'."));
			}
		}
	}
	for (const auto &[requirement_id, requirement] : requirements) {
		const auto evaluation = evaluations.find(requirement_id);
		if (evaluation == evaluations.end()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::MissingRequirementEvaluation,
				"Building requirement '" + requirement_id.value() +
					"' has no final evaluation."));
			continue;
		}
		const BuildingRequirementEvaluationStatus status = evaluation->second->status();
		if (requirement->criticality() == BuildingRequirementCriticality::Hard) {
			if (status == BuildingRequirementEvaluationStatus::Failed) {
				report.addIssue(BuildingModelValidationIssue(
					BuildingModelValidationCode::FailedHardRequirement,
					"Hard building requirement '" + requirement_id.value() + "' failed."));
			} else if (status != BuildingRequirementEvaluationStatus::Passed) {
				report.addIssue(BuildingModelValidationIssue(
					BuildingModelValidationCode::UnknownHardRequirement,
					"Hard building requirement '" + requirement_id.value() +
						"' is unresolved."));
			}
		}
		if (status == BuildingRequirementEvaluationStatus::Passed) {
			for (const BuildingRequirementId &dependency_id : requirement->dependencyIds()) {
				const auto dependency_evaluation = evaluations.find(dependency_id);
				if (dependency_evaluation == evaluations.end() ||
				    dependency_evaluation->second->status() !=
					    BuildingRequirementEvaluationStatus::Passed) {
					report.addIssue(BuildingModelValidationIssue(
						BuildingModelValidationCode::PassedRequirementDependsOnUnresolvedRequirement,
						"Passed building requirement depends on an unpassed prerequisite."));
				}
			}
		}
		const std::optional<bool> model_satisfied = evaluate_requirement_against_models(
			*requirement, *evaluation->second, spatial_model, classification_model,
			function_model, service_model, scenario_model, evaluations);
		if (model_satisfied.has_value() && !*model_satisfied &&
		    status == BuildingRequirementEvaluationStatus::Passed) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::RequirementEvaluationContradictsModel,
				"Passed building requirement '" + requirement_id.value() +
					"' contradicts the authoritative model state."));
		}
	}
	return report;
}
