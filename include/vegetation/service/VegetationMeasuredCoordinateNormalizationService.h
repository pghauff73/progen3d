#pragma once

#include "vegetation/model/VegetationMeasuredCoordinateNormalizationResult.h"
#include "vegetation/model/VegetationMeasuredPoint3d.h"

#include <string>

class VegetationMeasuredCoordinateNormalizationService
{
public:
	bool supportsCoordinateSystem(
		const std::string &source_coordinate_system) const;
	VegetationMeasuredCoordinateNormalizationResult normalize(
		const VegetationMeasuredPoint3d &source_point,
		const std::string &source_coordinate_system) const;
};
