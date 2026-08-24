#pragma once

#include "vegetation/model/PlantGrowthRequest.h"
#include "vegetation/model/PlantGrowthResult.h"
#include "vegetation/model/VegetationComplexityLimits.h"

class PlantGrowthService
{
public:
	explicit PlantGrowthService(
		VegetationComplexityLimits complexity_limits =
			VegetationComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	PlantGrowthResult resolve(const PlantGrowthRequest &request) const;

private:
	VegetationComplexityLimits complexity_limits_;
};
