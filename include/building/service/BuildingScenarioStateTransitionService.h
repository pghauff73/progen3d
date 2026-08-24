#pragma once

#include "building/model/BuildingComplianceState.h"
#include "building/model/BuildingConditionState.h"
#include "building/model/BuildingOperationalState.h"
#include "building/model/BuildingPlacementState.h"
#include "building/model/BuildingServiceAvailabilityState.h"

class BuildingScenarioStateTransitionService {
public:
	bool canTransition(BuildingPlacementState current_state,
	                   BuildingPlacementState requested_state) const;
	bool canTransition(BuildingOperationalState current_state,
	                   BuildingOperationalState requested_state) const;
	bool canTransition(BuildingConditionState current_state,
	                   BuildingConditionState requested_state) const;
	bool canTransition(BuildingComplianceState current_state,
	                   BuildingComplianceState requested_state) const;
	bool canTransition(BuildingServiceAvailabilityState current_state,
	                   BuildingServiceAvailabilityState requested_state) const;
};
