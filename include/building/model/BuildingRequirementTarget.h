#pragma once

#include "building/model/BuildingFunctionId.h"
#include "building/model/BuildingScenarioId.h"
#include "building/model/BuildingServicePortId.h"
#include "spatial/model/SpatialConnectionId.h"
#include "spatial/model/SpatialObjectId.h"

#include <string>
#include <utility>

enum class BuildingRequirementTargetKind {
	Object,
	Connection,
	Function,
	ServicePort,
	Scenario,
	WholeBuilding
};

class BuildingRequirementTarget {
public:
	static BuildingRequirementTarget object(SpatialObjectId object_id)
	{
		return BuildingRequirementTarget(
			BuildingRequirementTargetKind::Object, object_id.value());
	}

	static BuildingRequirementTarget connection(SpatialConnectionId connection_id)
	{
		return BuildingRequirementTarget(
			BuildingRequirementTargetKind::Connection, connection_id.value());
	}

	static BuildingRequirementTarget function(BuildingFunctionId function_id)
	{
		return BuildingRequirementTarget(
			BuildingRequirementTargetKind::Function, function_id.value());
	}

	static BuildingRequirementTarget servicePort(BuildingServicePortId port_id)
	{
		return BuildingRequirementTarget(
			BuildingRequirementTargetKind::ServicePort, port_id.value());
	}

	static BuildingRequirementTarget scenario(BuildingScenarioId scenario_id)
	{
		return BuildingRequirementTarget(
			BuildingRequirementTargetKind::Scenario, scenario_id.value());
	}

	static BuildingRequirementTarget wholeBuilding()
	{
		return BuildingRequirementTarget(BuildingRequirementTargetKind::WholeBuilding, {});
	}

	BuildingRequirementTargetKind kind() const { return kind_; }
	const std::string &identifier() const { return identifier_; }

private:
	BuildingRequirementTarget(BuildingRequirementTargetKind kind, std::string identifier)
		: kind_(kind), identifier_(std::move(identifier)) {}

	BuildingRequirementTargetKind kind_ = BuildingRequirementTargetKind::WholeBuilding;
	std::string identifier_;
};
