#pragma once

#include "geometry/service/ProceduralShapeMeshGenerator.h"

class AxialProfileMeshGenerator : public ProceduralShapeMeshGenerator
{
public:
	GeneratedPrimitiveMesh generate(
		const ShapeSpecification &specification,
		std::string *diagnostic) const override;
};
