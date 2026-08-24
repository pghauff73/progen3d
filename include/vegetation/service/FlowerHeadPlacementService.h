#pragma once

#include "vegetation/model/FlowerHeadSpecification.h"
#include "vegetation/model/InflorescencePlacement.h"
#include "vegetation/model/VegetationComplexityLimits.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>

class FlowerHeadPlacementService
{
public:
	explicit FlowerHeadPlacementService(
		VegetationComplexityLimits complexity_limits = VegetationComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::vector<InflorescencePlacement> place(
		const FlowerHeadSpecification &specification,
		const std::string &identifier_prefix,
		glm::vec3 origin,
		glm::vec3 axis,
		std::string *diagnostic = nullptr) const;

private:
	VegetationComplexityLimits complexity_limits_;
};
