#pragma once

#include "geometry/model/GeometryComplexityLimits.h"
#include "vehicle/model/SuspensionCornerSpecification.h"
#include "vehicle/model/VehicleAssemblyGeometry.h"

class SuspensionCornerAssemblyBuilder
{
public:
	explicit SuspensionCornerAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	VehicleAssemblyBuildResult build(
		const SuspensionCornerSpecification &specification) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
