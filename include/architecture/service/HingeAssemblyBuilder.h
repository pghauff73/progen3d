#pragma once

#include "architecture/model/ArchitecturalAssemblyBuildResult.h"
#include "architecture/model/HingeSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/GeometryDetailLevel.h"

class HingeAssemblyBuilder
{
public:
	explicit HingeAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	ArchitecturalAssemblyBuildResult build(
		const HingeSpecification &specification,
		GeometryDetailLevel detail_level =
			GeometryDetailLevel::FastenersAndSeals) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
