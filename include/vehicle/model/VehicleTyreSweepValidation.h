#pragma once

#include "geometry/model/ConvexHullMeshGenerationResult.h"
#include "geometry/model/MeshIntersectionScreeningReport.h"
#include "geometry/model/MeshSurfaceDistanceReport.h"
#include "vehicle/model/VehicleWheelPoseEvaluation.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

class VehicleTransformedTyrePoseEvidence
{
public:
	VehicleTransformedTyrePoseEvidence(
		VehicleWheelPoseEvaluation wheel_pose,
		std::size_t transformed_vertex_count,
		std::size_t transformed_face_count,
		std::uint64_t deterministic_geometry_hash,
		MeshSurfaceDistanceReport body_distance,
		MeshIntersectionScreeningReport intersection_screening,
		std::size_t parity_sample_count,
		std::size_t inside_sample_count,
		bool passed)
		: wheel_pose_(std::move(wheel_pose)),
		  transformed_vertex_count_(transformed_vertex_count),
		  transformed_face_count_(transformed_face_count),
		  deterministic_geometry_hash_(deterministic_geometry_hash),
		  body_distance_(std::move(body_distance)),
		  intersection_screening_(std::move(intersection_screening)),
		  parity_sample_count_(parity_sample_count),
		  inside_sample_count_(inside_sample_count),
		  passed_(passed)
	{
	}

	const VehicleWheelPoseEvaluation &wheelPose() const { return wheel_pose_; }
	std::size_t transformedVertexCount() const { return transformed_vertex_count_; }
	std::size_t transformedFaceCount() const { return transformed_face_count_; }
	std::uint64_t deterministicGeometryHash() const
	{
		return deterministic_geometry_hash_;
	}
	const MeshSurfaceDistanceReport &bodyDistance() const { return body_distance_; }
	const MeshIntersectionScreeningReport &intersectionScreening() const
	{
		return intersection_screening_;
	}
	std::size_t paritySampleCount() const { return parity_sample_count_; }
	std::size_t insideSampleCount() const { return inside_sample_count_; }
	bool passed() const { return passed_; }

private:
	VehicleWheelPoseEvaluation wheel_pose_{
		VehicleCornerLocation::FrontLeft, 0.0, 0.0, 0.0, 0.0, {}, {}};
	std::size_t transformed_vertex_count_ = 0u;
	std::size_t transformed_face_count_ = 0u;
	std::uint64_t deterministic_geometry_hash_ = 0u;
	MeshSurfaceDistanceReport body_distance_;
	MeshIntersectionScreeningReport intersection_screening_;
	std::size_t parity_sample_count_ = 0u;
	std::size_t inside_sample_count_ = 0u;
	bool passed_ = false;
};

class VehicleTyreSweepValidationReport
{
public:
	VehicleTyreSweepValidationReport(
		std::vector<VehicleTransformedTyrePoseEvidence> pose_evidence,
		ConvexHullMeshGenerationReport hull_report,
		double minimum_clearance_metres,
		bool passed)
		: pose_evidence_(std::move(pose_evidence)),
		  hull_report_(std::move(hull_report)),
		  minimum_clearance_metres_(minimum_clearance_metres),
		  passed_(passed)
	{
	}

	const std::vector<VehicleTransformedTyrePoseEvidence> &poseEvidence() const
	{
		return pose_evidence_;
	}
	const ConvexHullMeshGenerationReport &hullReport() const { return hull_report_; }
	double minimumClearanceMetres() const { return minimum_clearance_metres_; }
	bool passed() const { return passed_; }

private:
	std::vector<VehicleTransformedTyrePoseEvidence> pose_evidence_;
	ConvexHullMeshGenerationReport hull_report_{"", 0u, 0u, 0u, 0.0, false, false};
	double minimum_clearance_metres_ = 0.0;
	bool passed_ = false;
};
