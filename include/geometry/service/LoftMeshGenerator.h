#pragma once

#include "geometry/model/GeometryBuildResult.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/service/ProceduralShapeMeshGenerator.h"

class LoftMeshGenerator : public ProceduralShapeMeshGenerator
{
public:
	explicit LoftMeshGenerator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	GeometryBuildResult build(const ShapeSpecification &specification) const;
	GeneratedPrimitiveMesh generate(
		const ShapeSpecification &specification,
		std::string *diagnostic) const override;

private:
	GeometryComplexityLimits complexity_limits_;
};
