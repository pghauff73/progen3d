#pragma once

#include "vegetation/model/RuleBranchingGenerationRequest.h"
#include "vegetation/model/RuleBranchingGenerationResult.h"
#include "vegetation/model/VegetationComplexityLimits.h"

class RuleBranchingGenerationService
{
public:
	explicit RuleBranchingGenerationService(
		VegetationComplexityLimits complexity_limits =
			VegetationComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	RuleBranchingGenerationResult generate(
		const RuleBranchingGenerationRequest &request) const;

private:
	VegetationComplexityLimits complexity_limits_;
};

