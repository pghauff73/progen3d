#pragma once

#include "vegetation/model/SpaceColonizationRequest.h"
#include "vegetation/model/SpaceColonizationResult.h"
#include "vegetation/model/VegetationComplexityLimits.h"

class SpaceColonizationService
{
public:
	explicit SpaceColonizationService(
		VegetationComplexityLimits complexity_limits =
			VegetationComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	SpaceColonizationResult resolve(
		const SpaceColonizationRequest &request) const;

private:
	VegetationComplexityLimits complexity_limits_;
};
