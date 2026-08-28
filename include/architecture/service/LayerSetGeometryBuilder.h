#pragma once

#include "architecture/model/LayerSetBuildResult.h"
#include "architecture/model/LayerSetSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"

class LayerSetGeometryBuilder
{
public:
	explicit LayerSetGeometryBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	LayerSetBuildResult build(const LayerSetSpecification &specification) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
