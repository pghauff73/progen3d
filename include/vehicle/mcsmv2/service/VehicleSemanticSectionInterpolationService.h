#pragma once

#include "vehicle/mcsmv2/model/ShapePreservingCubicInterpolation.h"

#include <vector>

class VehicleSemanticSectionInterpolationService
{
public:
	ShapePreservingCubicInterpolation createInterpolation(
		const std::vector<double> &coordinates,
		const std::vector<double> &values) const;
};
