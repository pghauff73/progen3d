#pragma once

#include "building/model/BuildingFunction.h"
#include "building/relationship/BuildingFunctionAllocationRelationship.h"
#include "building/relationship/BuildingFunctionalDependencyRelationship.h"

#include <utility>
#include <vector>

class BuildingFunctionModel {
public:
	BuildingFunctionModel() = default;
	BuildingFunctionModel(
		std::vector<BuildingFunction> functions,
		std::vector<BuildingFunctionAllocationRelationship> allocations,
		std::vector<BuildingFunctionalDependencyRelationship> dependencies)
		: functions_(std::move(functions)),
		  allocations_(std::move(allocations)),
		  dependencies_(std::move(dependencies)) {}

	const std::vector<BuildingFunction> &functions() const { return functions_; }
	const std::vector<BuildingFunctionAllocationRelationship> &allocations() const
	{
		return allocations_;
	}
	const std::vector<BuildingFunctionalDependencyRelationship> &dependencies() const
	{
		return dependencies_;
	}

	const BuildingFunction *findFunction(const BuildingFunctionId &function_id) const
	{
		for (const BuildingFunction &function : functions_) {
			if (function.functionId() == function_id) return &function;
		}
		return nullptr;
	}

private:
	std::vector<BuildingFunction> functions_;
	std::vector<BuildingFunctionAllocationRelationship> allocations_;
	std::vector<BuildingFunctionalDependencyRelationship> dependencies_;
};
