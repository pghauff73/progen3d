#pragma once

#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/GeometryDetailLevel.h"
#include "vehicle/model/VehicleInteriorSpecification.h"
#include "vehicle/model/VehicleSubsystemAssemblySet.h"

class VehicleInteriorAssemblyBuilder
{
public:
	explicit VehicleInteriorAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	VehicleSubsystemAssemblyBuildResult build(
		const VehicleInteriorSpecification &specification,
		GeometryDetailLevel detail_level) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
