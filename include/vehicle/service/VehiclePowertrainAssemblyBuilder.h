#pragma once

#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/GeometryDetailLevel.h"
#include "vehicle/model/VehiclePowertrainSpecification.h"
#include "vehicle/model/VehicleSubsystemAssemblySet.h"

class VehiclePowertrainAssemblyBuilder
{
public:
	explicit VehiclePowertrainAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	VehicleSubsystemAssemblyBuildResult build(
		const VehiclePowertrainSpecification &specification,
		GeometryDetailLevel detail_level) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
