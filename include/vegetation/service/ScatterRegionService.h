#pragma once

#include "vegetation/model/ScatterRegionRequest.h"
#include "vegetation/model/ScatterRegionResult.h"
#include "vegetation/model/VegetationComplexityLimits.h"

class ScatterRegionService
{
public:
	explicit ScatterRegionService(
		VegetationComplexityLimits complexity_limits =
			VegetationComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	ScatterRegionResult resolve(const ScatterRegionRequest &request) const;

private:
	VegetationComplexityLimits complexity_limits_;
};
