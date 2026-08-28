#pragma once

#include "geometry/model/MeshIntersectionScreeningReport.h"
#include "geometry/model/MeshSurfaceDistanceReport.h"
#include "geometry/model/RigidTransformValidationReport.h"

#include <limits>
#include <utility>

class VehicleClosureAdjacencyExclusion
{
public:
	VehicleClosureAdjacencyExclusion(
		double moving_surface_axis_radius_metres,
		double fixed_surface_axis_radius_metres)
		: moving_surface_axis_radius_metres_(moving_surface_axis_radius_metres),
		  fixed_surface_axis_radius_metres_(fixed_surface_axis_radius_metres)
	{
	}

	double movingSurfaceAxisRadiusMetres() const
	{
		return moving_surface_axis_radius_metres_;
	}
	double fixedSurfaceAxisRadiusMetres() const
	{
		return fixed_surface_axis_radius_metres_;
	}

private:
	double moving_surface_axis_radius_metres_ = 0.0;
	double fixed_surface_axis_radius_metres_ = 0.0;
};

class VehicleClosurePoseValidation
{
public:
	VehicleClosurePoseValidation() = default;

	VehicleClosurePoseValidation(
		MeshSurfaceDistanceReport non_hinge_distance,
		MeshIntersectionScreeningReport intersection_screening,
		RigidTransformValidationReport rigid_transform,
		bool passed)
		: non_hinge_distance_(std::move(non_hinge_distance)),
		  intersection_screening_(std::move(intersection_screening)),
		  rigid_transform_(std::move(rigid_transform)),
		  passed_(passed)
	{
	}

	const MeshSurfaceDistanceReport &nonHingeDistance() const
	{
		return non_hinge_distance_;
	}
	const MeshIntersectionScreeningReport &intersectionScreening() const
	{
		return intersection_screening_;
	}
	const RigidTransformValidationReport &rigidTransform() const
	{
		return rigid_transform_;
	}
	bool passed() const { return passed_; }

private:
	MeshSurfaceDistanceReport non_hinge_distance_;
	MeshIntersectionScreeningReport intersection_screening_;
	RigidTransformValidationReport rigid_transform_{
		false, std::numeric_limits<double>::infinity(), 0.0,
		std::numeric_limits<double>::infinity(), false};
	bool passed_ = false;
};

class VehicleGlassPoseValidation
{
public:
	VehicleGlassPoseValidation() = default;

	VehicleGlassPoseValidation(
		double cavity_containment_fraction,
		MeshSurfaceDistanceReport fixed_body_distance,
		double support_pose_composition_error,
		MeshIntersectionScreeningReport intersection_screening,
		RigidTransformValidationReport rigid_transform,
		bool passed)
		: cavity_containment_fraction_(cavity_containment_fraction),
		  fixed_body_distance_(std::move(fixed_body_distance)),
		  support_pose_composition_error_(support_pose_composition_error),
		  intersection_screening_(std::move(intersection_screening)),
		  rigid_transform_(std::move(rigid_transform)),
		  passed_(passed)
	{
	}

	double cavityContainmentFraction() const
	{
		return cavity_containment_fraction_;
	}
	const MeshSurfaceDistanceReport &fixedBodyDistance() const
	{
		return fixed_body_distance_;
	}
	double supportPoseCompositionError() const
	{
		return support_pose_composition_error_;
	}
	const MeshIntersectionScreeningReport &intersectionScreening() const
	{
		return intersection_screening_;
	}
	const RigidTransformValidationReport &rigidTransform() const
	{
		return rigid_transform_;
	}
	bool passed() const { return passed_; }

private:
	double cavity_containment_fraction_ = 0.0;
	MeshSurfaceDistanceReport fixed_body_distance_;
	double support_pose_composition_error_ =
		std::numeric_limits<double>::infinity();
	MeshIntersectionScreeningReport intersection_screening_;
	RigidTransformValidationReport rigid_transform_{
		false, std::numeric_limits<double>::infinity(), 0.0,
		std::numeric_limits<double>::infinity(), false};
	bool passed_ = false;
};
