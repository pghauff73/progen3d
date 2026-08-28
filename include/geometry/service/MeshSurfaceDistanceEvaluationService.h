#pragma once

#include "Mesh.h"
#include "geometry/model/MeshSurfaceDistanceReport.h"
#include "geometry/service/TriangleGeometryRelationshipService.h"

class MeshSurfaceDistanceEvaluationService
{
public:
	MeshSurfaceDistanceReport evaluate(
		const Mesh &first_mesh,
		const Mesh &second_mesh) const;

private:
	TriangleGeometryRelationshipService triangle_relationship_service_;
};
