#pragma once

#include "geometry/service/MeshIntersectionScreeningService.h"
#include "geometry/service/MeshRigidTransformationService.h"
#include "geometry/service/MeshSurfaceDistanceEvaluationService.h"
#include "geometry/service/RigidTransformValidationService.h"
#include "vehicle/model/VehicleClosureMotion.h"
#include "vehicle/service/VehicleReferenceFrameTransformationService.h"

class VehicleGlassMotionValidationService
{
public:
	VehicleGlassMotion validateAndBuildMotion(
		const VehicleGlassMotion &unevaluated_motion,
		const GeneratedPrimitiveMesh &glass_mesh,
		const GeneratedPrimitiveMesh &parent_closure_mesh,
		const GeneratedPrimitiveMesh &fixed_body_mesh,
		const glm::dmat4 &parent_closure_source_transform,
		const glm::dmat4 &source_to_progen3d_matrix) const;

private:
	struct DoorCavityBounds
	{
		glm::dvec3 minimum{0.0};
		glm::dvec3 maximum{0.0};
	};

	DoorCavityBounds createDoorCavityBounds(
		const Mesh &parent_closure_mesh,
		const std::string &side) const;

	double containmentFraction(
		const Mesh &posed_glass_mesh,
		const DoorCavityBounds &cavity_bounds,
		double normalized_state) const;

	double maximumMatrixDifference(
		const glm::dmat4 &first,
		const glm::dmat4 &second) const;

	MeshRigidTransformationService transformation_service_;
	MeshSurfaceDistanceEvaluationService distance_service_;
	MeshIntersectionScreeningService intersection_service_;
	RigidTransformValidationService rigid_transform_service_;
	VehicleReferenceFrameTransformationService reference_frame_service_;
};
