#pragma once

#include <cstddef>

class VegetationWoodyBranchGraphReconstructionPolicy
{
public:
	VegetationWoodyBranchGraphReconstructionPolicy(
		std::size_t minimum_axis_point_count,
		std::size_t minimum_training_axis_point_count,
		std::size_t minimum_points_per_axial_station,
		std::size_t maximum_axis_count,
		std::size_t maximum_total_cylinder_count,
		std::size_t holdout_divisor,
		double minimum_assignment_confidence,
		double maximum_unassigned_woody_fraction,
		double minimum_principal_variance_fraction,
		double maximum_training_surface_rmse_metres,
		double maximum_holdout_surface_rmse_metres,
		double maximum_holdout_surface_residual_metres,
		double minimum_holdout_surface_coverage_fraction,
		double maximum_attachment_surface_gap_metres,
		double maximum_taper_violation_fraction,
		double minimum_observed_radius_metres,
		double minimum_axis_length_metres)
		: minimum_axis_point_count_(minimum_axis_point_count),
		  minimum_training_axis_point_count_(
			  minimum_training_axis_point_count),
		  minimum_points_per_axial_station_(
			  minimum_points_per_axial_station),
		  maximum_axis_count_(maximum_axis_count),
		  maximum_total_cylinder_count_(maximum_total_cylinder_count),
		  holdout_divisor_(holdout_divisor),
		  minimum_assignment_confidence_(minimum_assignment_confidence),
		  maximum_unassigned_woody_fraction_(
			  maximum_unassigned_woody_fraction),
		  minimum_principal_variance_fraction_(
			  minimum_principal_variance_fraction),
		  maximum_training_surface_rmse_metres_(
			  maximum_training_surface_rmse_metres),
		  maximum_holdout_surface_rmse_metres_(
			  maximum_holdout_surface_rmse_metres),
		  maximum_holdout_surface_residual_metres_(
			  maximum_holdout_surface_residual_metres),
		  minimum_holdout_surface_coverage_fraction_(
			  minimum_holdout_surface_coverage_fraction),
		  maximum_attachment_surface_gap_metres_(
			  maximum_attachment_surface_gap_metres),
		  maximum_taper_violation_fraction_(
			  maximum_taper_violation_fraction),
		  minimum_observed_radius_metres_(minimum_observed_radius_metres),
		  minimum_axis_length_metres_(minimum_axis_length_metres)
	{
	}

	std::size_t minimumAxisPointCount() const
	{
		return minimum_axis_point_count_;
	}
	std::size_t minimumTrainingAxisPointCount() const
	{
		return minimum_training_axis_point_count_;
	}
	std::size_t minimumPointsPerAxialStation() const
	{
		return minimum_points_per_axial_station_;
	}
	std::size_t maximumAxisCount() const { return maximum_axis_count_; }
	std::size_t maximumTotalCylinderCount() const
	{
		return maximum_total_cylinder_count_;
	}
	std::size_t holdoutDivisor() const { return holdout_divisor_; }
	double minimumAssignmentConfidence() const
	{
		return minimum_assignment_confidence_;
	}
	double maximumUnassignedWoodyFraction() const
	{
		return maximum_unassigned_woody_fraction_;
	}
	double minimumPrincipalVarianceFraction() const
	{
		return minimum_principal_variance_fraction_;
	}
	double maximumTrainingSurfaceRmseMetres() const
	{
		return maximum_training_surface_rmse_metres_;
	}
	double maximumHoldoutSurfaceRmseMetres() const
	{
		return maximum_holdout_surface_rmse_metres_;
	}
	double maximumHoldoutSurfaceResidualMetres() const
	{
		return maximum_holdout_surface_residual_metres_;
	}
	double minimumHoldoutSurfaceCoverageFraction() const
	{
		return minimum_holdout_surface_coverage_fraction_;
	}
	double maximumAttachmentSurfaceGapMetres() const
	{
		return maximum_attachment_surface_gap_metres_;
	}
	double maximumTaperViolationFraction() const
	{
		return maximum_taper_violation_fraction_;
	}
	double minimumObservedRadiusMetres() const
	{
		return minimum_observed_radius_metres_;
	}
	double minimumAxisLengthMetres() const
	{
		return minimum_axis_length_metres_;
	}

private:
	std::size_t minimum_axis_point_count_ = 0u;
	std::size_t minimum_training_axis_point_count_ = 0u;
	std::size_t minimum_points_per_axial_station_ = 0u;
	std::size_t maximum_axis_count_ = 0u;
	std::size_t maximum_total_cylinder_count_ = 0u;
	std::size_t holdout_divisor_ = 0u;
	double minimum_assignment_confidence_ = 0.0;
	double maximum_unassigned_woody_fraction_ = 0.0;
	double minimum_principal_variance_fraction_ = 0.0;
	double maximum_training_surface_rmse_metres_ = 0.0;
	double maximum_holdout_surface_rmse_metres_ = 0.0;
	double maximum_holdout_surface_residual_metres_ = 0.0;
	double minimum_holdout_surface_coverage_fraction_ = 0.0;
	double maximum_attachment_surface_gap_metres_ = 0.0;
	double maximum_taper_violation_fraction_ = 0.0;
	double minimum_observed_radius_metres_ = 0.0;
	double minimum_axis_length_metres_ = 0.0;
};
