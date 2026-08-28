#pragma once

#include "architecture/model/ArchitecturalAssemblyBuildResult.h"
#include "architecture/model/SofaSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/GeometryDetailLevel.h"

class SofaAssemblyBuilder
{
public:
	explicit SofaAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	ArchitecturalAssemblyBuildResult build(
		const SofaSpecification &specification,
		GeometryDetailLevel detail_level = GeometryDetailLevel::Component) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
