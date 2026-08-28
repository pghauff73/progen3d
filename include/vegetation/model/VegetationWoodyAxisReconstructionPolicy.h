#pragma once

#include <cstddef>

class VegetationWoodyAxisReconstructionPolicy
{
public:
	VegetationWoodyAxisReconstructionPolicy(
		double axial_station_spacing_metres,
		std::size_t minimum_woody_point_count,
		std::size_t minimum_training_woody_point_count,
		std::size_t minimum_points_per_axial_station,
		std::size_t maximum_cylinder_count,
		std::size_t holdout_divisor,
		double radial_outlier_mad_multiplier,
		double maximum_excluded_outlier_fraction,
		double minimum_woody_label_fraction,
		double minimum_principal_variance_fraction,
		double maximum_training_surface_rmse_metres,
		double maximum_holdout_surface_rmse_metres,
		double minimum_holdout_axial_coverage_fraction,
		double maximum_taper_violation_fraction,
		double minimum_observed_radius_metres)
		: axial_station_spacing_metres_(axial_station_spacing_metres),
		  minimum_woody_point_count_(minimum_woody_point_count),
		  minimum_training_woody_point_count_(
			  minimum_training_woody_point_count),
		  minimum_points_per_axial_station_(
			  minimum_points_per_axial_station),
		  maximum_cylinder_count_(maximum_cylinder_count),
		  holdout_divisor_(holdout_divisor),
		  radial_outlier_mad_multiplier_(radial_outlier_mad_multiplier),
		  maximum_excluded_outlier_fraction_(
			  maximum_excluded_outlier_fraction),
		  minimum_woody_label_fraction_(minimum_woody_label_fraction),
		  minimum_principal_variance_fraction_(
			  minimum_principal_variance_fraction),
		  maximum_training_surface_rmse_metres_(
			  maximum_training_surface_rmse_metres),
		  maximum_holdout_surface_rmse_metres_(
			  maximum_holdout_surface_rmse_metres),
		  minimum_holdout_axial_coverage_fraction_(
			  minimum_holdout_axial_coverage_fraction),
		  maximum_taper_violation_fraction_(
			  maximum_taper_violation_fraction),
		  minimum_observed_radius_metres_(minimum_observed_radius_metres)
	{
	}

	double axialStationSpacingMetres() const
	{
		return axial_station_spacing_metres_;
	}
	std::size_t minimumWoodyPointCount() const
	{
		return minimum_woody_point_count_;
	}
	std::size_t minimumTrainingWoodyPointCount() const
	{
		return minimum_training_woody_point_count_;
	}
	std::size_t minimumPointsPerAxialStation() const
	{
		return minimum_points_per_axial_station_;
	}
	std::size_t maximumCylinderCount() const
	{
		return maximum_cylinder_count_;
	}
	std::size_t holdoutDivisor() const { return holdout_divisor_; }
	double radialOutlierMadMultiplier() const
	{
		return radial_outlier_mad_multiplier_;
	}
	double maximumExcludedOutlierFraction() const
	{
		return maximum_excluded_outlier_fraction_;
	}
	double minimumWoodyLabelFraction() const
	{
		return minimum_woody_label_fraction_;
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
	double minimumHoldoutAxialCoverageFraction() const
	{
		return minimum_holdout_axial_coverage_fraction_;
	}
	double maximumTaperViolationFraction() const
	{
		return maximum_taper_violation_fraction_;
	}
	double minimumObservedRadiusMetres() const
	{
		return minimum_observed_radius_metres_;
	}

private:
	double axial_station_spacing_metres_ = 0.0;
	std::size_t minimum_woody_point_count_ = 0u;
	std::size_t minimum_training_woody_point_count_ = 0u;
	std::size_t minimum_points_per_axial_station_ = 0u;
	std::size_t maximum_cylinder_count_ = 0u;
	std::size_t holdout_divisor_ = 0u;
	double radial_outlier_mad_multiplier_ = 0.0;
	double maximum_excluded_outlier_fraction_ = 0.0;
	double minimum_woody_label_fraction_ = 0.0;
	double minimum_principal_variance_fraction_ = 0.0;
	double maximum_training_surface_rmse_metres_ = 0.0;
	double maximum_holdout_surface_rmse_metres_ = 0.0;
	double minimum_holdout_axial_coverage_fraction_ = 0.0;
	double maximum_taper_violation_fraction_ = 0.0;
	double minimum_observed_radius_metres_ = 0.0;
};
