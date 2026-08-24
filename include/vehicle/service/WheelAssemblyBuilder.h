#pragma once

#include "geometry/model/GeometryComplexityLimits.h"
#include "vehicle/model/VehicleAssemblyGeometry.h"
#include "vehicle/model/WheelAssemblySpecification.h"

class WheelAssemblyBuilder
{
public:
	explicit WheelAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	VehicleAssemblyBuildResult build(
		const WheelAssemblySpecification &specification) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
