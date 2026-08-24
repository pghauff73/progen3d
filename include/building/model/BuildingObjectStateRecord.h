#pragma once

#include "building/model/BuildingComplianceState.h"
#include "building/model/BuildingConditionState.h"
#include "building/model/BuildingOperationalState.h"
#include "building/model/BuildingPlacementState.h"
#include "building/model/BuildingServiceAvailabilityState.h"
#include "spatial/model/SpatialObjectId.h"

#include <utility>

class BuildingObjectStateRecord {
public:
	BuildingObjectStateRecord(SpatialObjectId object_id,
	                          BuildingPlacementState placement_state,
	                          BuildingOperationalState operational_state,
	                          BuildingConditionState condition_state,
	                          BuildingComplianceState compliance_state,
	                          BuildingServiceAvailabilityState service_availability_state)
		: object_id_(std::move(object_id)),
		  placement_state_(placement_state),
		  operational_state_(operational_state),
		  condition_state_(condition_state),
		  compliance_state_(compliance_state),
		  service_availability_state_(service_availability_state) {}

	const SpatialObjectId &objectId() const { return object_id_; }
	BuildingPlacementState placementState() const { return placement_state_; }
	BuildingOperationalState operationalState() const { return operational_state_; }
	BuildingConditionState conditionState() const { return condition_state_; }
	BuildingComplianceState complianceState() const { return compliance_state_; }
	BuildingServiceAvailabilityState serviceAvailabilityState() const
	{
		return service_availability_state_;
	}

private:
	SpatialObjectId object_id_;
	BuildingPlacementState placement_state_ = BuildingPlacementState::Unplaced;
	BuildingOperationalState operational_state_ = BuildingOperationalState::NotApplicable;
	BuildingConditionState condition_state_ = BuildingConditionState::NotApplicable;
	BuildingComplianceState compliance_state_ = BuildingComplianceState::NotEvaluated;
	BuildingServiceAvailabilityState service_availability_state_ =
		BuildingServiceAvailabilityState::NotApplicable;
};
