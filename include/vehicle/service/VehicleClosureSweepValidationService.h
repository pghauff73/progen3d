#pragma once

#include "geometry/service/MeshIntersectionScreeningService.h"
#include "geometry/service/MeshRigidTransformationService.h"
#include "geometry/service/MeshSurfaceDistanceEvaluationService.h"
#include "geometry/service/RigidTransformValidationService.h"
#include "vehicle/model/VehicleClosureMotion.h"
#include "vehicle/service/VehicleReferenceFrameTransformationService.h"

class VehicleClosureSweepValidationService
{
public:
	VehicleClosureSweep validateAndBuildSweep(
		const VehicleClosureSweep &unevaluated_sweep,
		const GeneratedPrimitiveMesh &closure_mesh,
		const GeneratedPrimitiveMesh &fixed_body_mesh,
		const glm::dmat4 &source_to_progen3d_matrix) const;

private:
	VehicleClosureAdjacencyExclusion createAdjacencyExclusion(
		const std::string &closure_identifier) const;

	GeneratedPrimitiveMesh excludeAxisZone(
		const GeneratedPrimitiveMesh &source_mesh,
		const glm::dvec3 &axis_point,
		const glm::dvec3 &axis_direction,
		double radius_metres) const;

	MeshRigidTransformationService transformation_service_;
	MeshSurfaceDistanceEvaluationService distance_service_;
	MeshIntersectionScreeningService intersection_service_;
	RigidTransformValidationService rigid_transform_service_;
	VehicleReferenceFrameTransformationService reference_frame_service_;
};
