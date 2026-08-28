#pragma once

#include "vegetation/model/PlantDevelopmentState.h"
#include "vegetation/model/PlantShapeSpecification.h"
#include "vegetation/model/PlantTopologyGenerationResult.h"
#include "vegetation/model/VegetationComplexityLimits.h"

class PlantTopologyGenerationService
{
public:
	explicit PlantTopologyGenerationService(
		VegetationComplexityLimits complexity_limits =
			VegetationComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	PlantTopologyGenerationResult generate(
		const PlantShapeSpecification &plant,
		float topology_age,
		PlantDevelopmentState topology_state) const;

private:
	VegetationComplexityLimits complexity_limits_;
};
