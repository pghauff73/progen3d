#include "building/service/BuildingScenarioStateTransitionService.h"

bool BuildingScenarioStateTransitionService::canTransition(
	BuildingPlacementState current_state,
	BuildingPlacementState requested_state) const
{
	if (current_state == requested_state) return true;
	if (requested_state == BuildingPlacementState::Invalid) return true;
	switch (current_state) {
	case BuildingPlacementState::Unplaced:
		return requested_state == BuildingPlacementState::Authored;
	case BuildingPlacementState::Authored:
		return requested_state == BuildingPlacementState::Resolved;
	case BuildingPlacementState::Resolved:
		return requested_state == BuildingPlacementState::ContactResolved;
	case BuildingPlacementState::ContactResolved:
	case BuildingPlacementState::Invalid:
		return false;
	}
	return false;
}

bool BuildingScenarioStateTransitionService::canTransition(
	BuildingOperationalState current_state,
	BuildingOperationalState requested_state) const
{
	if (current_state == requested_state) return true;
	if (current_state == BuildingOperationalState::NotApplicable ||
	    requested_state == BuildingOperationalState::NotApplicable) {
		return false;
	}
	if (requested_state == BuildingOperationalState::Failed) return true;
	if (current_state == BuildingOperationalState::Failed) {
		return requested_state == BuildingOperationalState::Unavailable;
	}
	return true;
}

bool BuildingScenarioStateTransitionService::canTransition(
	BuildingConditionState current_state,
	BuildingConditionState requested_state) const
{
	if (current_state == requested_state) return true;
	if (current_state == BuildingConditionState::NotApplicable ||
	    requested_state == BuildingConditionState::NotApplicable) {
		return false;
	}
	if (current_state == BuildingConditionState::Unknown) return true;
	if (current_state == BuildingConditionState::Serviceable) {
		return requested_state == BuildingConditionState::Degraded ||
		       requested_state == BuildingConditionState::Unserviceable;
	}
	if (current_state == BuildingConditionState::Degraded) {
		return requested_state == BuildingConditionState::Serviceable ||
		       requested_state == BuildingConditionState::Unserviceable;
	}
	return requested_state == BuildingConditionState::Degraded ||
	       requested_state == BuildingConditionState::Serviceable;
}

bool BuildingScenarioStateTransitionService::canTransition(
	BuildingComplianceState current_state,
	BuildingComplianceState requested_state) const
{
	if (current_state == requested_state) return true;
	if (current_state == BuildingComplianceState::NotApplicable ||
	    requested_state == BuildingComplianceState::NotApplicable) {
		return false;
	}
	return true;
}

bool BuildingScenarioStateTransitionService::canTransition(
	BuildingServiceAvailabilityState current_state,
	BuildingServiceAvailabilityState requested_state) const
{
	if (current_state == requested_state) return true;
	if (current_state == BuildingServiceAvailabilityState::NotApplicable ||
	    requested_state == BuildingServiceAvailabilityState::NotApplicable) {
		return false;
	}
	return true;
}
