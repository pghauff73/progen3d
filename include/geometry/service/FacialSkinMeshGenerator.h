#pragma once

#include "geometry/model/FacialSkinLandmarks.h"
#include "geometry/service/ProceduralShapeMeshGenerator.h"

class FacialSkinShapeSpecification;

class FacialSkinMeshGenerator : public ProceduralShapeMeshGenerator
{
public:
	GeneratedPrimitiveMesh generate(
		const ShapeSpecification &specification,
		std::string *diagnostic) const override;

	FacialSkinLandmarks evaluateLandmarks(
		const FacialSkinShapeSpecification &specification) const;
};
