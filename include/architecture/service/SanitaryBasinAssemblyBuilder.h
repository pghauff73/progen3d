#pragma once

#include "architecture/model/ArchitecturalAssemblyBuildResult.h"
#include "architecture/model/SanitaryBasinSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"

class SanitaryBasinAssemblyBuilder
{
public:
	explicit SanitaryBasinAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	ArchitecturalAssemblyBuildResult build(
		const SanitaryBasinSpecification &specification) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
