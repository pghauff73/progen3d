#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/model/GeometryBuildResult.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/InstanceArraySpecification.h"

class InstanceArrayGeometryBuilder
{
public:
	explicit InstanceArrayGeometryBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	GeometryBuildResult build(
		const GeneratedPrimitiveMesh &source,
		const InstanceArraySpecification &array) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
