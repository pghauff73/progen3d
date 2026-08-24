#pragma once

#include "architecture/model/ArchitecturalAssemblyBuildResult.h"
#include "architecture/model/PoolAssemblySpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"

class PoolAssemblyBuilder
{
public:
	explicit PoolAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	ArchitecturalAssemblyBuildResult build(
		const PoolAssemblySpecification &specification) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
