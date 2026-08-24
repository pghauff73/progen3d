#pragma once

#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/GeometryDetailLevel.h"
#include "vehicle/model/VehicleBodySpecification.h"
#include "vehicle/model/VehicleDefinition.h"
#include "vehicle/model/VehicleSubsystemAssemblySet.h"

class VehicleExteriorAssemblyBuilder
{
public:
	explicit VehicleExteriorAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	VehicleSubsystemAssemblyBuildResult build(
		const VehicleDefinition &definition,
		const VehicleBodySpecification &body_specification,
		GeometryDetailLevel detail_level) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
