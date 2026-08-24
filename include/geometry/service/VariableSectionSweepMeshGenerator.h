#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/model/GeometryBuildResult.h"
#include "geometry/model/GeometryComplexityLimits.h"

#include <string>

class ShapeSpecification;

class VariableSectionSweepMeshGenerator
{
public:
	explicit VariableSectionSweepMeshGenerator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	GeometryBuildResult build(const ShapeSpecification &specification) const;
	GeneratedPrimitiveMesh generate(
		const ShapeSpecification &specification,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
