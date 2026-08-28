#include "building/service/SmallModernBuildingModelValidationService.h"

#include "building/service/BuildingClassificationValidationService.h"
#include "building/service/BuildingFunctionCoverageEvaluationService.h"
#include "building/service/BuildingRequirementEvaluationService.h"
#include "building/service/BuildingRelationshipAssertionValidationService.h"
#include "building/service/BuildingScenarioValidationService.h"
#include "building/service/BuildingServiceTopologyValidationService.h"
#include "spatial/model/SpatialBuildingModel.h"

#include <chrono>
#include <set>

namespace {

bool evidence_exists(
	const BuildingEvidenceReference &reference,
	const BuildingEvidenceLedger &ledger)
{
	return !reference.empty() && ledger.find(reference.evidenceId()) != nullptr;
}

void validate_evidence_reference(
	const BuildingEvidenceReference &reference,
	const BuildingEvidenceLedger &ledger,
	BuildingModelValidationReport *report)
{
	if (report != nullptr && !evidence_exists(reference, ledger)) {
		report->addIssue(BuildingModelValidationIssue(
			BuildingModelValidationCode::UndefinedEvidenceReference,
			"Building model record references undefined evidence."));
	}
}

bool source_string_exceeds_limit(
	const SmallModernBuildingModelConstructionRequest &request,
	std::size_t maximum_length)
{
	auto exceeds = [maximum_length](const std::string &value) {
		return value.size() > maximum_length;
	};
	if (exceeds(request.sourceSpatialSchema()) ||
	    exceeds(request.sourceRootObjectId().value())) return true;
	for (const SpatialObjectId &object_id : request.expectedObjectIds()) {
		if (exceeds(object_id.value())) return true;
	}
	for (const BuildingConceptDefinition &concept_definition :
	     request.classificationModel().concepts()) {
		if (exceeds(concept_definition.conceptId().value()) ||
		    exceeds(concept_definition.canonicalName())) {
			return true;
		}
		if (concept_definition.broaderConceptId().has_value() &&
		    exceeds(concept_definition.broaderConceptId()->value())) return true;
		for (const std::string &alias : concept_definition.acceptedAliases()) {
			if (exceeds(alias)) return true;
		}
	}
	for (const BuildingRoleDefinition &role : request.classificationModel().roles()) {
		if (exceeds(role.roleId().value()) || exceeds(role.canonicalName()) ||
		    exceeds(role.purpose())) return true;
	}
	for (const BuildingFunction &function : request.functionModel().functions()) {
		if (exceeds(function.functionId().value()) || exceeds(function.purpose())) {
			return true;
		}
	}
	for (const BuildingServiceSystem &system : request.serviceModel().systems()) {
		if (exceeds(system.systemId().value()) || exceeds(system.ownerObjectId().value()) ||
		    exceeds(system.purpose())) return true;
	}
	for (const BuildingRelationshipAssertionReference &assertion :
	     request.relationshipAssertionModel().assertions()) {
		if (exceeds(assertion.assertionObjectId().value()) ||
		    exceeds(assertion.canonicalFactId().value()) ||
		    exceeds(assertion.sourceObjectId().value()) ||
		    exceeds(assertion.targetObjectId().value())) return true;
	}
	for (const auto &requirement : request.requirementModel().requirements()) {
		if (!requirement) continue;
		if (exceeds(requirement->requirementId().value()) ||
		    exceeds(requirement->target().identifier())) return true;
		for (const BuildingRequirementId &dependency_id : requirement->dependencyIds()) {
			if (exceeds(dependency_id.value())) return true;
		}
	}
	for (const BuildingRequirementEvaluationRecord &evaluation :
	     request.requirementModel().evaluationRecords()) {
		if (exceeds(evaluation.requirementId().value()) ||
		    exceeds(evaluation.measuredValue().description()) ||
		    exceeds(evaluation.requiredValue().description())) return true;
		for (const std::string &diagnostic : evaluation.diagnostics()) {
			if (exceeds(diagnostic)) return true;
		}
	}
	for (const BuildingScenario &scenario : request.scenarioModel().scenarios()) {
		if (exceeds(scenario.scenarioId().value()) || exceeds(scenario.canonicalName()) ||
		    exceeds(scenario.purpose())) return true;
	}
	for (const BuildingStateTransitionRecord &transition :
	     request.scenarioModel().transitions()) {
		if (exceeds(transition.stateFacet()) || exceeds(transition.previousState()) ||
		    exceeds(transition.requestedState())) return true;
	}
	for (const BuildingEvidenceRecord &evidence : request.evidenceLedger().records()) {
		if (exceeds(evidence.evidenceId().value()) ||
		    exceeds(evidence.sourceIdentifier()) || exceeds(evidence.summary())) return true;
	}
	return false;
}

} // namespace

BuildingModelValidationReport SmallModernBuildingModelValidationService::validate(
	const SpatialBuildingModel &spatial_model,
	const SmallModernBuildingModelConstructionRequest &request,
	const BuildingModelSafetyLimits &limits) const
{
	return validate(spatial_model, request, limits, nullptr);
}

BuildingModelValidationReport SmallModernBuildingModelValidationService::validate(
	const SpatialBuildingModel &spatial_model,
	const SmallModernBuildingModelConstructionRequest &request,
	const BuildingModelSafetyLimits &limits,
	double *requirement_evaluation_milliseconds) const
{
	BuildingModelValidationReport report;
	if (source_string_exceeds_limit(request, limits.maximum_source_string_length)) {
		report.addIssue(BuildingModelValidationIssue(
			BuildingModelValidationCode::SafetyCeilingExceeded,
			"Building model source string exceeds the configured safety ceiling."));
	}
	if (request.sourceSpatialSchema() != "SMB-OMv2-SpatialObjectModel-v1") {
		report.addIssue(BuildingModelValidationIssue(
			BuildingModelValidationCode::InvalidIdentifier,
			"SMB-OMv2.1 source spatial schema does not match the accepted contract."));
	}
	if (request.sourceRootObjectId() != spatial_model.containmentTree().rootObjectId()) {
		report.addIssue(BuildingModelValidationIssue(
			BuildingModelValidationCode::UndefinedProfileObject,
			"SMB-OMv2.1 source root object does not match the spatial model."));
	}
	std::set<SpatialObjectId> expected_object_ids(
		request.expectedObjectIds().begin(), request.expectedObjectIds().end());
	std::set<SpatialObjectId> actual_object_ids;
	for (const SpatialBuildingObject &object : spatial_model.objects().objects()) {
		actual_object_ids.insert(object.identity().objectId());
	}
	if (expected_object_ids.size() != request.expectedObjectIds().size() ||
	    expected_object_ids != actual_object_ids) {
		report.addIssue(BuildingModelValidationIssue(
			BuildingModelValidationCode::MissingObjectProfile,
			"SMB-OMv2.1 generated object set does not exactly match the spatial model."));
	}

	const BuildingEvidenceLedger &ledger = request.evidenceLedger();
	if (ledger.records().size() > limits.maximum_evidence_records) {
		report.addIssue(BuildingModelValidationIssue(
			BuildingModelValidationCode::SafetyCeilingExceeded,
			"Building evidence ledger exceeds the configured safety ceiling."));
	}
	std::set<BuildingEvidenceId> evidence_ids;
	for (const BuildingEvidenceRecord &evidence : ledger.records()) {
		if (evidence.evidenceId().empty() || evidence.sourceIdentifier().empty() ||
		    evidence.summary().empty() || evidence.evidenceHash() == 0 ||
		    !evidence_ids.insert(evidence.evidenceId()).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::InvalidEvidenceHash,
				"Building evidence records require unique IDs, sources, summaries, and hashes."));
		}
	}
	for (const BuildingObjectRoleAssignment &assignment :
	     request.classificationModel().roleAssignments()) {
		validate_evidence_reference(assignment.evidence(), ledger, &report);
	}
	for (const BuildingObjectSemanticProfile &profile :
	     request.classificationModel().objectProfiles()) {
		if (profile.profileHash() == 0) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::InvalidEvidenceHash,
				"Building semantic profile has no deterministic hash.",
				{profile.objectId()}));
		}
		for (const BuildingEvidenceReference &reference : profile.evidenceReferences()) {
			validate_evidence_reference(reference, ledger, &report);
		}
	}
	for (const BuildingFunctionAllocationRelationship &allocation :
	     request.functionModel().allocations()) {
		validate_evidence_reference(allocation.evidence(), ledger, &report);
	}
	for (const BuildingServicePort &port : request.serviceModel().ports()) {
		validate_evidence_reference(port.evidence(), ledger, &report);
	}
	for (const BuildingRelationshipAssertionReference &assertion :
	     request.relationshipAssertionModel().assertions()) {
		validate_evidence_reference(assertion.evidence(), ledger, &report);
	}
	for (const BuildingServiceFlow &flow : request.serviceModel().flows()) {
		validate_evidence_reference(flow.evidence(), ledger, &report);
	}
	for (const BuildingStateTransitionRecord &transition :
	     request.scenarioModel().transitions()) {
		validate_evidence_reference(transition.evidence(), ledger, &report);
	}

	report.append(BuildingClassificationValidationService().validate(
		request.classificationModel(), spatial_model, limits));
	report.append(BuildingFunctionCoverageEvaluationService().validate(
		request.functionModel(), spatial_model, limits));
	report.append(BuildingServiceTopologyValidationService().validate(
		request.serviceModel(), spatial_model, limits));
	report.append(BuildingRelationshipAssertionValidationService().validate(
		request.relationshipAssertionModel(), spatial_model, limits));
	report.append(BuildingScenarioValidationService().validate(
		request.scenarioModel(), spatial_model, limits));
	const auto requirement_evaluation_begin = std::chrono::steady_clock::now();
	report.append(BuildingRequirementEvaluationService().validateFinalEvaluations(
		request.requirementModel(), spatial_model, request.classificationModel(),
		request.functionModel(), request.serviceModel(), request.scenarioModel(),
		request.evidenceLedger(), limits));
	const auto requirement_evaluation_end = std::chrono::steady_clock::now();
	if (requirement_evaluation_milliseconds != nullptr) {
		*requirement_evaluation_milliseconds =
			std::chrono::duration<double, std::milli>(
				requirement_evaluation_end - requirement_evaluation_begin).count();
	}
	report.enforceMaximumIssueCount(limits.maximum_diagnostic_count);
	return report;
}
