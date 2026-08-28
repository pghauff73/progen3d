#pragma once

#include "building/model/BuildingFunctionId.h"

#include <utility>

class BuildingFunctionalDependencyRelationship {
public:
	BuildingFunctionalDependencyRelationship(BuildingFunctionId prerequisite_function_id,
	                                         BuildingFunctionId dependent_function_id)
		: prerequisite_function_id_(std::move(prerequisite_function_id)),
		  dependent_function_id_(std::move(dependent_function_id)) {}

	const BuildingFunctionId &prerequisiteFunctionId() const
	{
		return prerequisite_function_id_;
	}
	const BuildingFunctionId &dependentFunctionId() const
	{
		return dependent_function_id_;
	}

private:
	BuildingFunctionId prerequisite_function_id_;
	BuildingFunctionId dependent_function_id_;
};
