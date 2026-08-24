#pragma once

#include "vegetation/model/VineGrowthRequest.h"
#include "vegetation/model/VineGrowthResult.h"
#include "vegetation/model/VegetationComplexityLimits.h"

class VineGrowthService
{
public:
	explicit VineGrowthService(
		VegetationComplexityLimits complexity_limits =
			VegetationComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	VineGrowthResult resolve(const VineGrowthRequest &request) const;

private:
	VegetationComplexityLimits complexity_limits_;
};
