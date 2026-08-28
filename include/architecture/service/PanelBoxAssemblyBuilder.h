#pragma once

#include "architecture/model/ArchitecturalAssemblyBuildResult.h"
#include "architecture/model/PanelBoxSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"

class PanelBoxAssemblyBuilder
{
public:
	explicit PanelBoxAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	ArchitecturalAssemblyBuildResult build(
		const PanelBoxSpecification &specification) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
