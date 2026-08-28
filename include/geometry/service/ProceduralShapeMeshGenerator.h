#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "geometry/model/ShapeSpecification.h"

#include <string>

class ProceduralShapeMeshGenerator
{
public:
	virtual ~ProceduralShapeMeshGenerator() = default;

	virtual GeneratedPrimitiveMesh generate(
		const ShapeSpecification &specification,
		std::string *diagnostic) const = 0;
};
