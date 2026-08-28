#pragma once

#include "vegetation/model/SpaceColonizationSpecification.h"
#include "vegetation/model/VegetationComplexityLimits.h"

#include <string>

class SpaceColonizationSpecificationValidationService
{
public:
	explicit SpaceColonizationSpecificationValidationService(
		VegetationComplexityLimits complexity_limits =
			VegetationComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	bool validate(
		const SpaceColonizationSpecification &specification,
		std::string *diagnostic = nullptr) const;

private:
	VegetationComplexityLimits complexity_limits_;
};
