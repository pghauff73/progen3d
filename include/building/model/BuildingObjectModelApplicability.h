#pragma once

#include "building/model/BuildingModelApplicability.h"

class BuildingObjectModelApplicability {
public:
	BuildingObjectModelApplicability(
		BuildingModelApplicability geometry,
		BuildingModelApplicability spatial_boundary,
		BuildingModelApplicability spatial_interfaces,
		BuildingModelApplicability functional_roles,
		BuildingModelApplicability building_functions,
		BuildingModelApplicability service_ports,
		BuildingModelApplicability requirements,
		BuildingModelApplicability operational_state,
		BuildingModelApplicability condition_state,
		BuildingModelApplicability scenario_participation)
		: geometry_(geometry),
		  spatial_boundary_(spatial_boundary),
		  spatial_interfaces_(spatial_interfaces),
		  functional_roles_(functional_roles),
		  building_functions_(building_functions),
		  service_ports_(service_ports),
		  requirements_(requirements),
		  operational_state_(operational_state),
		  condition_state_(condition_state),
		  scenario_participation_(scenario_participation) {}

	BuildingModelApplicability geometry() const { return geometry_; }
	BuildingModelApplicability spatialBoundary() const { return spatial_boundary_; }
	BuildingModelApplicability spatialInterfaces() const { return spatial_interfaces_; }
	BuildingModelApplicability functionalRoles() const { return functional_roles_; }
	BuildingModelApplicability buildingFunctions() const { return building_functions_; }
	BuildingModelApplicability servicePorts() const { return service_ports_; }
	BuildingModelApplicability requirements() const { return requirements_; }
	BuildingModelApplicability operationalState() const { return operational_state_; }
	BuildingModelApplicability conditionState() const { return condition_state_; }
	BuildingModelApplicability scenarioParticipation() const
	{
		return scenario_participation_;
	}

private:
	BuildingModelApplicability geometry_ = BuildingModelApplicability::NotApplicable;
	BuildingModelApplicability spatial_boundary_ = BuildingModelApplicability::NotApplicable;
	BuildingModelApplicability spatial_interfaces_ = BuildingModelApplicability::NotApplicable;
	BuildingModelApplicability functional_roles_ = BuildingModelApplicability::Applicable;
	BuildingModelApplicability building_functions_ = BuildingModelApplicability::NotApplicable;
	BuildingModelApplicability service_ports_ = BuildingModelApplicability::NotApplicable;
	BuildingModelApplicability requirements_ = BuildingModelApplicability::Applicable;
	BuildingModelApplicability operational_state_ = BuildingModelApplicability::NotApplicable;
	BuildingModelApplicability condition_state_ = BuildingModelApplicability::NotApplicable;
	BuildingModelApplicability scenario_participation_ = BuildingModelApplicability::NotApplicable;
};
