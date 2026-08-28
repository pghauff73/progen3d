#pragma once

#include "architecture/model/ArchitecturalAssemblyBuildResult.h"
#include "architecture/model/DrawerSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/GeometryDetailLevel.h"

class DrawerAssemblyBuilder
{
public:
	explicit DrawerAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	ArchitecturalAssemblyBuildResult build(
		const DrawerSpecification &specification,
		GeometryDetailLevel detail_level = GeometryDetailLevel::Component) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
