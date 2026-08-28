#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/service/ProceduralShapeMeshGenerator.h"

class ScatterRegionMeshGenerator : public ProceduralShapeMeshGenerator
{
public:
	GeneratedPrimitiveMesh generate(
		const ShapeSpecification &specification,
		std::string *diagnostic) const override;
};
