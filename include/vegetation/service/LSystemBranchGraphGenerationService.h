#pragma once

#include "vegetation/model/LSystemBranchGraphGenerationResult.h"
#include "vegetation/model/PlantDevelopmentState.h"
#include "vegetation/model/PlantLSystemSpecification.h"
#include "vegetation/model/TropismInfluence.h"
#include "vegetation/model/VegetationComplexityLimits.h"

#include <vector>

class LSystemBranchGraphGenerationService
{
public:
	explicit LSystemBranchGraphGenerationService(
		VegetationComplexityLimits complexity_limits =
			VegetationComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	LSystemBranchGraphGenerationResult generate(
		const PlantLSystemSpecification &specification,
		float developmental_age,
		PlantDevelopmentState development_state,
		const std::vector<TropismInfluence> &tropism_influences = {}) const;

private:
	VegetationComplexityLimits complexity_limits_;
};
