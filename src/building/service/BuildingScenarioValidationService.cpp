#include "building/service/BuildingScenarioValidationService.h"

#include "building/service/BuildingScenarioStateTransitionService.h"
#include "building/service/BuildingStateSnapshotHashService.h"

#include "spatial/model/SpatialBuildingModel.h"

#include <set>

namespace {

bool legal_transition(const BuildingStateTransitionRecord &transition)
{
	const BuildingScenarioStateTransitionService service;
	if (transition.stateFacet() == "Placement") {
		if (transition.previousState() == "Unplaced" &&
		    transition.requestedState() == "Authored") {
			return service.canTransition(
				BuildingPlacementState::Unplaced, BuildingPlacementState::Authored);
		}
		if (transition.previousState() == "Authored" &&
		    transition.requestedState() == "Resolved") {
			return service.canTransition(
				BuildingPlacementState::Authored, BuildingPlacementState::Resolved);
		}
		if (transition.previousState() == "Resolved" &&
		    transition.requestedState() == "ContactResolved") {
			return service.canTransition(
				BuildingPlacementState::Resolved,
				BuildingPlacementState::ContactResolved);
		}
		return false;
	}
	if (transition.stateFacet() == "Operational") {
		if (transition.previousState() == "Available" &&
		    transition.requestedState() == "Operating") {
			return service.canTransition(
				BuildingOperationalState::Available,
				BuildingOperationalState::Operating);
		}
		if (transition.previousState() == "Operating" &&
		    transition.requestedState() == "Unavailable") {
			return service.canTransition(
				BuildingOperationalState::Operating,
				BuildingOperationalState::Unavailable);
		}
		return false;
	}
	if (transition.stateFacet() == "Condition") {
		if (transition.previousState() == "Serviceable" &&
		    transition.requestedState() == "Degraded") {
			return service.canTransition(
				BuildingConditionState::Serviceable,
				BuildingConditionState::Degraded);
		}
		if (transition.previousState() == "Degraded" &&
		    transition.requestedState() == "Unserviceable") {
			return service.canTransition(
				BuildingConditionState::Degraded,
				BuildingConditionState::Unserviceable);
		}
		return false;
	}
	if (transition.stateFacet() == "Compliance") {
		if (transition.previousState() == "NotEvaluated" &&
		    transition.requestedState() == "Passed") {
			return service.canTransition(
				BuildingComplianceState::NotEvaluated,
				BuildingComplianceState::Passed);
		}
		return false;
	}
	if (transition.stateFacet() == "ServiceAvailability") {
		if (transition.previousState() == "Available" &&
		    transition.requestedState() == "Unavailable") {
			return service.canTransition(
				BuildingServiceAvailabilityState::Available,
				BuildingServiceAvailabilityState::Unavailable);
		}
		if (transition.previousState() == "Unavailable" &&
		    transition.requestedState() == "Available") {
			return service.canTransition(
				BuildingServiceAvailabilityState::Unavailable,
				BuildingServiceAvailabilityState::Available);
		}
		return false;
	}
	return false;
}

} // namespace

BuildingModelValidationReport BuildingScenarioValidationService::validate(
	const BuildingScenarioModel &scenario_model,
	const SpatialBuildingModel &spatial_model,
	const BuildingModelSafetyLimits &limits) const
{
	BuildingModelValidationReport report;
	if (scenario_model.scenarios().size() > limits.maximum_scenarios) {
		report.addIssue(BuildingModelValidationIssue(
			BuildingModelValidationCode::SafetyCeilingExceeded,
			"Building scenario model exceeds the configured safety ceiling."));
	}

	std::set<BuildingScenarioId> scenario_ids;
	for (const BuildingScenario &scenario : scenario_model.scenarios()) {
		if (scenario.scenarioId().empty() || scenario.canonicalName().empty() ||
		    !scenario_ids.insert(scenario.scenarioId()).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::InvalidScenario,
				"Building scenarios require unique nonempty IDs and canonical names."));
		}
	}

	std::set<std::pair<BuildingScenarioId, SpatialObjectId>> participation_keys;
	for (const BuildingScenarioParticipation &participation :
	     scenario_model.participations()) {
		if (scenario_ids.find(participation.scenarioId()) == scenario_ids.end() ||
		    spatial_model.objects().find(participation.objectId()) == nullptr ||
		    !participation_keys.emplace(
			    participation.scenarioId(), participation.objectId()).second) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::InvalidScenario,
				"Building scenario participation references an undefined or duplicate fact.",
				{participation.objectId()}));
		}
	}

	for (const BuildingStateSnapshot &snapshot : scenario_model.snapshots()) {
		if (scenario_ids.find(snapshot.scenarioId()) == scenario_ids.end()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::InvalidScenario,
				"Building state snapshot references an invalid scenario."));
		}
		if (snapshot.snapshotHash() == 0 ||
		    snapshot.snapshotHash() !=
			    BuildingStateSnapshotHashService().calculate(snapshot)) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::InvalidEvidenceHash,
				"Building state snapshot hash does not match its immutable state records."));
		}
		std::set<SpatialObjectId> state_object_ids;
		for (const BuildingObjectStateRecord &state : snapshot.objectStates()) {
			if (spatial_model.objects().find(state.objectId()) == nullptr ||
			    !state_object_ids.insert(state.objectId()).second) {
				report.addIssue(BuildingModelValidationIssue(
					BuildingModelValidationCode::InvalidScenario,
					"Building state snapshot contains an undefined or duplicate object state.",
					{state.objectId()}));
			}
		}
		if (state_object_ids.size() != spatial_model.objects().size()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::InvalidScenario,
				"Building state snapshot does not cover every spatial object."));
		}
	}

	for (const BuildingStateTransitionRecord &transition :
	     scenario_model.transitions()) {
		if (scenario_ids.find(transition.scenarioId()) == scenario_ids.end() ||
		    spatial_model.objects().find(transition.objectId()) == nullptr ||
		    transition.stateFacet().empty() || transition.previousState().empty() ||
		    transition.requestedState().empty() || transition.evidence().empty()) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::InvalidStateTransition,
				"Building state transition is incomplete or references undefined records.",
				{transition.objectId()}));
		} else if (!legal_transition(transition)) {
			report.addIssue(BuildingModelValidationIssue(
				BuildingModelValidationCode::InvalidStateTransition,
				"Building state transition is not permitted by the state transition authority.",
				{transition.objectId()}));
		}
	}
	return report;
}
