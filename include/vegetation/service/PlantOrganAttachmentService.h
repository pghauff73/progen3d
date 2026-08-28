#pragma once

#include "vegetation/model/BranchGraph.h"
#include "vegetation/model/PlantSpeciesSpecification.h"
#include "vegetation/model/VegetationComplexityLimits.h"

#include <optional>
#include <string>

class PlantOrganAttachmentService
{
public:
	explicit PlantOrganAttachmentService(
		VegetationComplexityLimits complexity_limits =
			VegetationComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::optional<BranchGraph> attachLeaves(
		const BranchGraph &graph,
		const PlantSpeciesSpecification &species,
		float developmental_age,
		std::string *diagnostic = nullptr) const;

private:
	VegetationComplexityLimits complexity_limits_;
};
