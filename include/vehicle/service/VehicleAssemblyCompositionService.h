#pragma once

#include "geometry/model/GeometryBuildResult.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "vehicle/model/VehicleAssemblyGeometry.h"

#include <vector>

class VehicleAssemblyCompositionService
{
public:
	explicit VehicleAssemblyCompositionService(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	GeometryBuildResult compose(
		const std::vector<VehicleAssemblyPart> &parts) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
