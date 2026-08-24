#pragma once

#include "vegetation/model/PlantLSystemSpecification.h"
#include "vegetation/model/VegetationComplexityLimits.h"

#include <string>

class PlantLSystemSpecificationValidationService
{
public:
	explicit PlantLSystemSpecificationValidationService(
		VegetationComplexityLimits complexity_limits =
			VegetationComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	bool validate(
		const PlantLSystemSpecification &specification,
		std::string *diagnostic = nullptr) const;

private:
	VegetationComplexityLimits complexity_limits_;
};
