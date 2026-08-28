#pragma once

#include "vegetation/model/BranchGraph.h"
#include "vegetation/model/PlantSpaceColonizationSpecification.h"
#include "vegetation/model/PlantSpeciesSpecification.h"

#include <optional>
#include <string>

class SpaceColonizationSeedGraphFactory
{
public:
	std::optional<BranchGraph> create(
		const PlantSpeciesSpecification &species,
		const PlantSpaceColonizationSpecification &space_colonization,
		std::string *diagnostic = nullptr) const;
};
