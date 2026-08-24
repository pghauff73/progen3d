#pragma once

#include "geometry/model/GeneratedMeshPlacement.h"
#include "geometry/model/GeometryBuildResult.h"
#include "geometry/model/GeometryComplexityLimits.h"

#include <vector>

class GeneratedMeshComposer
{
public:
	explicit GeneratedMeshComposer(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	GeometryBuildResult compose(
		const std::vector<GeneratedMeshPlacement> &placements) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
