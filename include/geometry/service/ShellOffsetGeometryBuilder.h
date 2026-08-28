#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/model/GeometryBuildResult.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/ShellOffsetShapeSpecification.h"

class ShellOffsetGeometryBuilder
{
public:
	explicit ShellOffsetGeometryBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	GeometryBuildResult build(
		const GeneratedPrimitiveMesh &source,
		const ShellOffsetShapeSpecification &specification) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
