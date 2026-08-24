#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "vehicle/model/SuspensionCornerSpecification.h"
#include "vehicle/model/VehicleWheelPoseEvaluation.h"
#include "vehicle/model/VehicleTyreSweepValidation.h"

#include <glm/glm.hpp>

#include <utility>
#include <vector>

class VehicleTyrePoseSample
{
public:
	VehicleTyrePoseSample(
		VehicleWheelPoseEvaluation wheel_pose,
		glm::dmat4 progen3d_transform)
		: wheel_pose_(std::move(wheel_pose)),
		  progen3d_transform_(progen3d_transform)
	{
	}

	const VehicleWheelPoseEvaluation &wheelPose() const { return wheel_pose_; }
	const glm::dmat4 &progen3dTransform() const { return progen3d_transform_; }

private:
	VehicleWheelPoseEvaluation wheel_pose_{
		VehicleCornerLocation::FrontLeft, 0.0, 0.0, 0.0, 0.0, {}, {}};
	glm::dmat4 progen3d_transform_{1.0};
};

class VehicleTyrePoseSweep
{
public:
	VehicleTyrePoseSweep(
		VehicleCornerLocation corner,
		GeneratedPrimitiveMesh immutable_source_tyre_mesh,
		std::vector<VehicleTyrePoseSample> pose_samples,
		double accepted_minimum_clearance_metres,
		double tessellation_tolerance_metres,
		bool source_clearance_passed)
		: corner_(corner),
		  immutable_source_tyre_mesh_(std::move(immutable_source_tyre_mesh)),
		  pose_samples_(std::move(pose_samples)),
		  accepted_minimum_clearance_metres_(accepted_minimum_clearance_metres),
		  tessellation_tolerance_metres_(tessellation_tolerance_metres),
		  source_clearance_passed_(source_clearance_passed)
	{
	}

	VehicleTyrePoseSweep(
		VehicleCornerLocation corner,
		GeneratedPrimitiveMesh immutable_source_tyre_mesh,
		std::vector<VehicleTyrePoseSample> pose_samples,
		GeneratedPrimitiveMesh conservative_hull_mesh,
		VehicleTyreSweepValidationReport validation_report,
		double accepted_minimum_clearance_metres,
		double tessellation_tolerance_metres,
		bool source_clearance_passed)
		: corner_(corner),
		  immutable_source_tyre_mesh_(std::move(immutable_source_tyre_mesh)),
		  pose_samples_(std::move(pose_samples)),
		  conservative_hull_mesh_(std::move(conservative_hull_mesh)),
		  validation_report_(std::move(validation_report)),
		  accepted_minimum_clearance_metres_(accepted_minimum_clearance_metres),
		  tessellation_tolerance_metres_(tessellation_tolerance_metres),
		  source_clearance_passed_(source_clearance_passed)
	{
	}

	VehicleCornerLocation corner() const { return corner_; }
	const GeneratedPrimitiveMesh &immutableSourceTyreMesh() const
	{
		return immutable_source_tyre_mesh_;
	}
	const std::vector<VehicleTyrePoseSample> &poseSamples() const
	{
		return pose_samples_;
	}
	double acceptedMinimumClearanceMetres() const
	{
		return accepted_minimum_clearance_metres_;
	}
	double tessellationToleranceMetres() const
	{
		return tessellation_tolerance_metres_;
	}
	bool sourceClearancePassed() const { return source_clearance_passed_; }
	const GeneratedPrimitiveMesh &conservativeHullMesh() const
	{
		return conservative_hull_mesh_;
	}
	const VehicleTyreSweepValidationReport &validationReport() const
	{
		return validation_report_;
	}

private:
	VehicleCornerLocation corner_ = VehicleCornerLocation::FrontLeft;
	GeneratedPrimitiveMesh immutable_source_tyre_mesh_{std::make_shared<Mesh>(), {}};
	std::vector<VehicleTyrePoseSample> pose_samples_;
	GeneratedPrimitiveMesh conservative_hull_mesh_{std::make_shared<Mesh>(), {}};
	VehicleTyreSweepValidationReport validation_report_{{}, {"", 0u, 0u, 0u, 0.0, false, false}, 0.0, false};
	double accepted_minimum_clearance_metres_ = 0.0;
	double tessellation_tolerance_metres_ = 0.0;
	bool source_clearance_passed_ = false;
};

class VehicleTyreSweepSet
{
public:
	explicit VehicleTyreSweepSet(std::vector<VehicleTyrePoseSweep> sweeps)
		: sweeps_(std::move(sweeps))
	{
	}

	const std::vector<VehicleTyrePoseSweep> &sweeps() const { return sweeps_; }
	std::size_t totalPoseCount() const
	{
		std::size_t count = 0u;
		for (const VehicleTyrePoseSweep &sweep : sweeps_) {
			count += sweep.poseSamples().size();
		}
		return count;
	}

private:
	std::vector<VehicleTyrePoseSweep> sweeps_;
};
