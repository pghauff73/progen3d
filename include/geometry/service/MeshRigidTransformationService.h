#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"

#include <glm/glm.hpp>

class MeshRigidTransformationService
{
public:
	GeneratedPrimitiveMesh transform(
		const GeneratedPrimitiveMesh &source_mesh,
		const glm::dmat4 &rigid_transform) const;
};
