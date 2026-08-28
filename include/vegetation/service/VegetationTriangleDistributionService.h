#pragma once

#include "vegetation/model/VegetationTriangleDistributionRequest.h"
#include "vegetation/model/VegetationTriangleDistributionResult.h"

class VegetationTriangleDistributionService
{
public:
	VegetationTriangleDistributionResult distribute(
		const VegetationTriangleDistributionRequest &request) const;
};
