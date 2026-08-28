#pragma once

#include "building/relationship/BuildingFunctionalDependencyRelationship.h"

#include <utility>
#include <vector>

class BuildingFunctionGraph {
public:
	BuildingFunctionGraph() = default;
	explicit BuildingFunctionGraph(
		std::vector<BuildingFunctionalDependencyRelationship> dependencies)
		: dependencies_(std::move(dependencies)) {}

	const std::vector<BuildingFunctionalDependencyRelationship> &dependencies() const
	{
		return dependencies_;
	}

private:
	std::vector<BuildingFunctionalDependencyRelationship> dependencies_;
};
