#pragma once

#include "architecture/model/ArchitecturalAssemblyBuildResult.h"
#include "architecture/model/StairAssemblySpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"

class StairAssemblyBuilder
{
public:
	explicit StairAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	ArchitecturalAssemblyBuildResult build(
		const StairAssemblySpecification &specification) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
