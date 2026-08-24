#pragma once

#include "vegetation/model/PlantSpeciesSpecification.h"

#include <string>
#include <vector>

class PlantSpeciesCatalog
{
public:
	const std::vector<PlantSpeciesSpecification> &species() const;
	const PlantSpeciesSpecification *find(const std::string &identifier) const;
	std::vector<std::string> identifiers() const;
};
