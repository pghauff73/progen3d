#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/service/ProceduralShapeMeshGenerator.h"

class VineMeshGenerator : public ProceduralShapeMeshGenerator
{
public:
	GeneratedPrimitiveMesh generate(
		const ShapeSpecification &specification,
		std::string *diagnostic) const override;
};
