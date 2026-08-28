#pragma once

#include "geometry/service/ConvexHullMeshGenerationService.h"
#include "geometry/service/MeshIntersectionScreeningService.h"
#include "geometry/service/MeshRayParityClassificationService.h"
#include "geometry/service/MeshSurfaceDistanceEvaluationService.h"
#include "vehicle/model/VehicleTyrePoseSweep.h"
#include "vehicle/service/VehicleTyrePoseTransformationService.h"

class VehicleTyreSweepValidationService
{
public:
	VehicleTyrePoseSweep validateAndBuildSweep(
		VehicleCornerLocation corner,
		const GeneratedPrimitiveMesh &source_tyre_mesh,
		std::vector<VehicleTyrePoseSample> pose_samples,
		const Mesh &final_body_mesh,
		const glm::dmat4 &source_to_progen3d_matrix,
		double declared_clearance_metres,
		double accepted_minimum_clearance_metres,
		double tessellation_tolerance_metres) const;

private:
	VehicleTyrePoseTransformationService transformation_service_;
	MeshSurfaceDistanceEvaluationService distance_service_;
	MeshIntersectionScreeningService intersection_service_;
	MeshRayParityClassificationService parity_service_;
	ConvexHullMeshGenerationService hull_service_;
};
