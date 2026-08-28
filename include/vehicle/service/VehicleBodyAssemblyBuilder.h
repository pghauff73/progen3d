#pragma once

#include "geometry/model/GeometryComplexityLimits.h"
#include "vehicle/model/VehicleAssemblyGeometry.h"
#include "vehicle/model/VehicleBodySpecification.h"

class VehicleBodyAssemblyBuilder
{
public:
	explicit VehicleBodyAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	VehicleAssemblyBuildResult build(
		const VehicleBodySpecification &specification) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
