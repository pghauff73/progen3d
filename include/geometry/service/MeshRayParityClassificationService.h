#pragma once

#include "Mesh.h"
#include "geometry/model/MeshRayParityClassification.h"
#include "geometry/service/TriangleGeometryRelationshipService.h"

#include <glm/glm.hpp>

class MeshRayParityClassificationService
{
public:
	MeshRayParityClassification classify(
		const Mesh &closed_mesh,
		const glm::dvec3 &point,
		double surface_tolerance = 1.0e-7) const;

private:
	TriangleGeometryRelationshipService triangle_relationship_service_;
};
