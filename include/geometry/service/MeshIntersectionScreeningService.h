#pragma once

#include "Mesh.h"
#include "geometry/model/MeshIntersectionScreeningReport.h"
#include "geometry/service/TriangleGeometryRelationshipService.h"

class MeshIntersectionScreeningService
{
public:
	MeshIntersectionScreeningReport screen(
		const Mesh &first_mesh,
		const Mesh &second_mesh,
		double tolerance = 1.0e-8) const;

private:
	TriangleGeometryRelationshipService triangle_relationship_service_;
};
