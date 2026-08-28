#pragma once

#include "architecture/model/ArchitecturalAssemblyBuildResult.h"
#include "architecture/model/PanelArraySpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"

class PanelArrayAssemblyBuilder
{
public:
	explicit PanelArrayAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	ArchitecturalAssemblyBuildResult build(
		const PanelArraySpecification &specification) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
