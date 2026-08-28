#pragma once

#include "architecture/model/ArchitecturalAssemblyBuildResult.h"
#include "architecture/model/GlazingUnitSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"

class GlazingUnitAssemblyBuilder
{
public:
	explicit GlazingUnitAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	ArchitecturalAssemblyBuildResult build(
		const GlazingUnitSpecification &specification) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
