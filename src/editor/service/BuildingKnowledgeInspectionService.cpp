#include "editor/service/BuildingKnowledgeInspectionService.h"

#include "building/model/SmallModernBuildingModel.h"
#include "editor/model/SpatialObjectInspection.h"

#include <algorithm>
#include <set>
#include <sstream>

namespace {

const char *applicability_name(BuildingModelApplicability applicability)
{
	switch (applicability) {
	case BuildingModelApplicability::Applicable: return "Applicable";
	case BuildingModelApplicability::NotApplicable: return "Not applicable";
	case BuildingModelApplicability::PendingEvidence: return "Pending evidence";
	}
	return "Unknown";
}

const char *function_criticality_name(BuildingFunctionCriticality criticality)
{
	switch (criticality) {
	case BuildingFunctionCriticality::Hard: return "Hard";
	case BuildingFunctionCriticality::Soft: return "Soft";
	case BuildingFunctionCriticality::Informational: return "Informational";
	}
	return "Unknown";
}

const char *requirement_criticality_name(BuildingRequirementCriticality criticality)
{
	switch (criticality) {
	case BuildingRequirementCriticality::Hard: return "Hard";
	case BuildingRequirementCriticality::Soft: return "Soft";
	case BuildingRequirementCriticality::Informational: return "Informational";
	}
	return "Unknown";
}

const char *requirement_status_name(BuildingRequirementEvaluationStatus status)
{
	switch (status) {
	case BuildingRequirementEvaluationStatus::Passed: return "Passed";
	case BuildingRequirementEvaluationStatus::Failed: return "Failed";
	case BuildingRequirementEvaluationStatus::Unknown: return "Unknown";
	case BuildingRequirementEvaluationStatus::NotEvaluated: return "Not evaluated";
	case BuildingRequirementEvaluationStatus::NotApplicable: return "Not applicable";
	}
	return "Unknown";
}

const char *service_medium_name(BuildingServiceMedium medium)
{
	switch (medium) {
	case BuildingServiceMedium::ElectricalPower: return "Electrical power";
	case BuildingServiceMedium::PotableColdWater: return "Potable cold water";
	case BuildingServiceMedium::PotableHotWater: return "Potable hot water";
	case BuildingServiceMedium::WasteWater: return "Waste water";
	case BuildingServiceMedium::ConditionedAir: return "Conditioned air";
	case BuildingServiceMedium::ExhaustAir: return "Exhaust air";
	case BuildingServiceMedium::RefrigerantCircuit: return "Refrigerant circuit";
	case BuildingServiceMedium::SmokeMonitoringSignal: return "Smoke monitoring signal";
	case BuildingServiceMedium::ControlSignal: return "Control signal";
	case BuildingServiceMedium::Rainwater: return "Rainwater";
	}
	return "Unknown";
}

const char *service_direction_name(BuildingServiceFlowDirection direction)
{
	switch (direction) {
	case BuildingServiceFlowDirection::Source: return "Source";
	case BuildingServiceFlowDirection::Sink: return "Sink";
	case BuildingServiceFlowDirection::Bidirectional: return "Bidirectional";
	}
	return "Unknown";
}

const char *relationship_assertion_kind_name(BuildingRelationshipAssertionKind kind)
{
	switch (kind) {
	case BuildingRelationshipAssertionKind::Containment: return "Containment";
	case BuildingRelationshipAssertionKind::SpatialConnection: return "Spatial connection";
	}
	return "Unknown";
}

const char *placement_state_name(BuildingPlacementState state)
{
	switch (state) {
	case BuildingPlacementState::Unplaced: return "Unplaced";
	case BuildingPlacementState::Authored: return "Authored";
	case BuildingPlacementState::Resolved: return "Resolved";
	case BuildingPlacementState::ContactResolved: return "Contact resolved";
	case BuildingPlacementState::Invalid: return "Invalid";
	}
	return "Unknown";
}

const char *operational_state_name(BuildingOperationalState state)
{
	switch (state) {
	case BuildingOperationalState::NotApplicable: return "Not applicable";
	case BuildingOperationalState::Unavailable: return "Unavailable";
	case BuildingOperationalState::Available: return "Available";
	case BuildingOperationalState::Operating: return "Operating";
	case BuildingOperationalState::Failed: return "Failed";
	case BuildingOperationalState::Unknown: return "Unknown";
	}
	return "Unknown";
}

const char *condition_state_name(BuildingConditionState state)
{
	switch (state) {
	case BuildingConditionState::NotApplicable: return "Not applicable";
	case BuildingConditionState::Unknown: return "Unknown";
	case BuildingConditionState::Serviceable: return "Serviceable";
	case BuildingConditionState::Degraded: return "Degraded";
	case BuildingConditionState::Unserviceable: return "Unserviceable";
	}
	return "Unknown";
}

const char *compliance_state_name(BuildingComplianceState state)
{
	switch (state) {
	case BuildingComplianceState::NotEvaluated: return "Not evaluated";
	case BuildingComplianceState::Passed: return "Passed";
	case BuildingComplianceState::Failed: return "Failed";
	case BuildingComplianceState::Unknown: return "Unknown";
	case BuildingComplianceState::NotApplicable: return "Not applicable";
	}
	return "Unknown";
}

const char *service_availability_state_name(BuildingServiceAvailabilityState state)
{
	switch (state) {
	case BuildingServiceAvailabilityState::NotApplicable: return "Not applicable";
	case BuildingServiceAvailabilityState::Disconnected: return "Disconnected";
	case BuildingServiceAvailabilityState::Connected: return "Connected";
	case BuildingServiceAvailabilityState::Unavailable: return "Unavailable";
	case BuildingServiceAvailabilityState::Available: return "Available";
	case BuildingServiceAvailabilityState::Unknown: return "Unknown";
	}
	return "Unknown";
}

std::string taxonomy_path(
	const BuildingClassificationModel &classification_model,
	const BuildingConceptDefinition &leaf_concept)
{
	std::vector<std::string> names;
	std::set<BuildingConceptId> visited;
	const BuildingConceptDefinition *current = &leaf_concept;
	while (current != nullptr && visited.insert(current->conceptId()).second) {
		names.push_back(current->canonicalName());
		current = current->broaderConceptId().has_value()
			? classification_model.findConcept(*current->broaderConceptId())
			: nullptr;
	}
	std::reverse(names.begin(), names.end());
	std::ostringstream text;
	for (std::size_t index = 0; index < names.size(); ++index) {
		if (index > 0) text << " > ";
		text << names[index];
	}
	return text.str();
}

const BuildingObjectStateRecord *resolved_state_for(
	const BuildingScenarioModel &scenario_model,
	const SpatialObjectId &object_id)
{
	for (auto snapshot = scenario_model.snapshots().rbegin();
	     snapshot != scenario_model.snapshots().rend(); ++snapshot) {
		const auto state = std::find_if(
			snapshot->objectStates().begin(), snapshot->objectStates().end(),
			[&](const BuildingObjectStateRecord &record) {
				return record.objectId() == object_id;
			});
		if (state != snapshot->objectStates().end()) return &*state;
	}
	return nullptr;
}

} // namespace

void BuildingKnowledgeInspectionService::appendKnowledge(
	const SmallModernBuildingModel &building_model,
	const SpatialObjectId &object_id,
	SpatialObjectInspection *inspection) const
{
	if (inspection == nullptr) return;
	const BuildingClassificationModel &classification =
		building_model.classificationModel();
	const BuildingObjectSemanticProfile *profile = classification.findProfile(object_id);
	if (profile == nullptr) return;

	inspection->building_knowledge_available = true;
	inspection->semantic_profile_hash = profile->profileHash();
	inspection->building_model_hash = building_model.modelHash();
	const BuildingConceptDefinition *concept_definition =
		classification.findConcept(profile->conceptId());
	if (concept_definition != nullptr) {
		inspection->canonical_concept_id = concept_definition->conceptId().value();
		inspection->canonical_concept_name = concept_definition->canonicalName();
		inspection->semantic_taxonomy_path =
			taxonomy_path(classification, *concept_definition);
	}

	const BuildingObjectModelApplicability &applicability = profile->applicability();
	inspection->building_applicability = {
		std::string("Geometry: ") + applicability_name(applicability.geometry()),
		std::string("Spatial boundary: ") + applicability_name(applicability.spatialBoundary()),
		std::string("Spatial interfaces: ") + applicability_name(applicability.spatialInterfaces()),
		std::string("Functional roles: ") + applicability_name(applicability.functionalRoles()),
		std::string("Building functions: ") + applicability_name(applicability.buildingFunctions()),
		std::string("Service ports: ") + applicability_name(applicability.servicePorts()),
		std::string("Requirements: ") + applicability_name(applicability.requirements()),
		std::string("Operational state: ") + applicability_name(applicability.operationalState()),
		std::string("Condition state: ") + applicability_name(applicability.conditionState()),
		std::string("Scenarios: ") + applicability_name(applicability.scenarioParticipation())};

	for (const BuildingRoleId &role_id : profile->roleIds()) {
		const BuildingRoleDefinition *role = classification.findRole(role_id);
		inspection->building_roles.push_back(
			role != nullptr ? role->canonicalName() + " — " + role->purpose()
			                : role_id.value());
	}
	for (const BuildingFunctionAllocationId &allocation_id :
	     profile->functionAllocationIds()) {
		for (const BuildingFunctionAllocationRelationship &allocation :
		     building_model.functionModel().allocations()) {
			if (allocation.allocationId() != allocation_id) continue;
			const BuildingFunction *function =
				building_model.functionModel().findFunction(allocation.functionId());
			inspection->building_functions.push_back(
				function != nullptr
					? function->functionId().value() + " — " + function->purpose() +
						" [" + function_criticality_name(function->criticality()) + "]"
					: allocation.functionId().value());
		}
	}

	std::set<BuildingServicePortId> object_port_ids;
	for (const BuildingServicePortId &port_id : profile->servicePortIds()) {
		object_port_ids.insert(port_id);
		const BuildingServicePort *port = building_model.serviceModel().findPort(port_id);
		inspection->building_service_ports.push_back(
			port != nullptr
				? port->portId().value() + " — " + service_medium_name(port->medium()) +
					" [" + service_direction_name(port->direction()) + ", " +
					applicability_name(port->applicability()) + "]"
				: port_id.value());
	}
	for (const BuildingServiceFlow &flow : building_model.serviceModel().flows()) {
		if (object_port_ids.count(flow.sourcePortId()) != 0) {
			inspection->outgoing_service_flows.push_back(
				flow.flowId().value() + " -> " + flow.targetPortId().value());
		}
		if (object_port_ids.count(flow.targetPortId()) != 0) {
			inspection->incoming_service_flows.push_back(
				flow.flowId().value() + " <- " + flow.sourcePortId().value());
		}
	}
	const BuildingRelationshipAssertionReference *assertion =
		building_model.relationshipAssertionModel().findAssertion(object_id);
	if (assertion != nullptr) {
		inspection->building_relationship_assertions.push_back(
			std::string(relationship_assertion_kind_name(assertion->assertionKind())) +
			" — " + assertion->canonicalFactId().value() + " — " +
			assertion->sourceObjectId().value() + " -> " +
			assertion->targetObjectId().value());
	}

	for (const BuildingRequirementId &requirement_id : profile->requirementIds()) {
		const BuildingRequirement *requirement =
			building_model.requirementModel().findRequirement(requirement_id);
		const BuildingRequirementEvaluationRecord *evaluation =
			building_model.requirementModel().findEvaluation(requirement_id);
		std::string text = requirement_id.value();
		if (requirement != nullptr) {
			text += " [";
			text += requirement_criticality_name(requirement->criticality());
			text += "]";
		}
		text += " — ";
		text += evaluation != nullptr
			? requirement_status_name(evaluation->status())
			: "Missing evaluation";
		inspection->building_requirements.push_back(std::move(text));
	}
	for (const BuildingScenarioId &scenario_id : profile->scenarioIds()) {
		std::string text = scenario_id.value();
		for (const BuildingScenario &scenario : building_model.scenarioModel().scenarios()) {
			if (scenario.scenarioId() == scenario_id) {
				text = scenario.canonicalName() + " — " + scenario.purpose();
				break;
			}
		}
		inspection->building_scenarios.push_back(std::move(text));
	}
	for (const BuildingEvidenceReference &reference : profile->evidenceReferences()) {
		const BuildingEvidenceRecord *evidence =
			building_model.evidenceLedger().find(reference.evidenceId());
		inspection->building_evidence.push_back(
			evidence != nullptr
				? evidence->evidenceId().value() + " — " + evidence->summary()
				: reference.evidenceId().value());
	}

	const BuildingObjectStateRecord *state =
		resolved_state_for(building_model.scenarioModel(), object_id);
	if (state != nullptr) {
		inspection->placement_state = placement_state_name(state->placementState());
		inspection->operational_state = operational_state_name(state->operationalState());
		inspection->condition_state = condition_state_name(state->conditionState());
		inspection->compliance_state = compliance_state_name(state->complianceState());
		inspection->service_availability_state =
			service_availability_state_name(state->serviceAvailabilityState());
	}
}
