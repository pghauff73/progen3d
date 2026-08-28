#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "vehicle/model/VehicleTyreMeshSpecification.h"

#include <glm/glm.hpp>

class VehicleTyreMeshGenerationService
{
public:
	GeneratedPrimitiveMesh generateSourceFrameTyre(
		const VehicleTyreMeshSpecification &specification) const;

	GeneratedPrimitiveMesh generateTransformedTyre(
		const VehicleTyreMeshSpecification &specification,
		const glm::dmat4 &source_transform) const;

private:
	void validateSpecification(
		const VehicleTyreMeshSpecification &specification) const;
};
