#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/model/GeometryBuildResult.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/MirrorShapeSpecification.h"

class MirroredShapeGeometryBuilder
{
public:
	explicit MirroredShapeGeometryBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	GeometryBuildResult build(
		const GeneratedPrimitiveMesh &source,
		const MirrorShapeSpecification &specification) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
