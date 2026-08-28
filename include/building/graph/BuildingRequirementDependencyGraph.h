#pragma once

#include "building/model/BuildingRequirement.h"

#include <memory>
#include <utility>
#include <vector>

class BuildingRequirementDependencyGraph {
public:
	BuildingRequirementDependencyGraph() = default;
	explicit BuildingRequirementDependencyGraph(
		std::vector<std::shared_ptr<const BuildingRequirement>> requirements)
		: requirements_(std::move(requirements)) {}

	const std::vector<std::shared_ptr<const BuildingRequirement>> &requirements() const
	{
		return requirements_;
	}

private:
	std::vector<std::shared_ptr<const BuildingRequirement>> requirements_;
};
