#pragma once

#include "vegetation/model/VegetationMeasurementUnitNormalizationResult.h"

#include <string>

class VegetationMeasurementUnitNormalizationService
{
public:
	VegetationMeasurementUnitNormalizationResult normalize(
		double source_value,
		const std::string &source_unit,
		VegetationMeasurementQuantityKind quantity_kind) const;
};
