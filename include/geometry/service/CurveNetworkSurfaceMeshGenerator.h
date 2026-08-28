#pragma once

#include "geometry/model/GeometryBuildResult.h"
#include "geometry/service/ProceduralShapeMeshGenerator.h"

class CurveNetworkSurfaceMeshGenerator : public ProceduralShapeMeshGenerator
{
public:
	GeometryBuildResult build(const ShapeSpecification &specification) const;
	GeneratedPrimitiveMesh generate(
		const ShapeSpecification &specification,
		std::string *diagnostic) const override;
};
