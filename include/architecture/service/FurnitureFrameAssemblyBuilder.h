#pragma once

#include "architecture/model/ArchitecturalAssemblyBuildResult.h"
#include "architecture/model/FurnitureFrameSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/GeometryDetailLevel.h"

class FurnitureFrameAssemblyBuilder
{
public:
	explicit FurnitureFrameAssemblyBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	ArchitecturalAssemblyBuildResult build(
		const FurnitureFrameSpecification &specification,
		GeometryDetailLevel detail_level = GeometryDetailLevel::Assembly) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
