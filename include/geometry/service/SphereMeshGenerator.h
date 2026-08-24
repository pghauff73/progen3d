#pragma once

#include "geometry/service/ProceduralShapeMeshGenerator.h"

class SphereMeshGenerator : public ProceduralShapeMeshGenerator
{
public:
	GeneratedPrimitiveMesh generate(
		const ShapeSpecification &specification,
		std::string *diagnostic) const override;
};
