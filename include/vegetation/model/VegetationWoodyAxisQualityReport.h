#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class VegetationWoodyAxisQualityReport
{
public:
	VegetationWoodyAxisQualityReport(
		std::string reconstruction_identifier,
		std::string dataset_identifier,
		std::string source_payload_sha256,
		std::size_t training_woody_point_count,
		std::size_t holdout_woody_point_count,
		std::size_t excluded_training_outlier_count,
		std::size_t radius_station_count,
		std::size_t cylinder_count,
		double woody_label_fraction,
		double excluded_training_outlier_fraction,
		double principal_variance_fraction,
		double training_surface_rmse_metres,
		double holdout_surface_rmse_metres,
		double holdout_axial_coverage_fraction,
		double taper_violation_fraction,
		std::vector<std::string> rejection_reasons)
		: reconstruction_identifier_(std::move(reconstruction_identifier)),
		  dataset_identifier_(std::move(dataset_identifier)),
		  source_payload_sha256_(std::move(source_payload_sha256)),
		  training_woody_point_count_(training_woody_point_count),
		  holdout_woody_point_count_(holdout_woody_point_count),
		  excluded_training_outlier_count_(
			  excluded_training_outlier_count),
		  radius_station_count_(radius_station_count),
		  cylinder_count_(cylinder_count),
		  woody_label_fraction_(woody_label_fraction),
		  excluded_training_outlier_fraction_(
			  excluded_training_outlier_fraction),
		  principal_variance_fraction_(principal_variance_fraction),
		  training_surface_rmse_metres_(training_surface_rmse_metres),
		  holdout_surface_rmse_metres_(holdout_surface_rmse_metres),
		  holdout_axial_coverage_fraction_(
			  holdout_axial_coverage_fraction),
		  taper_violation_fraction_(taper_violation_fraction),
		  rejection_reasons_(std::move(rejection_reasons))
	{
	}

	bool acceptedForPrimaryAxisGeometry() const
	{
		return rejection_reasons_.empty();
	}
	const std::string &reconstructionIdentifier() const
	{
		return reconstruction_identifier_;
	}
	const std::string &datasetIdentifier() const { return dataset_identifier_; }
	const std::string &sourcePayloadSha256() const
	{
		return source_payload_sha256_;
	}
	std::size_t trainingWoodyPointCount() const
	{
		return training_woody_point_count_;
	}
	std::size_t holdoutWoodyPointCount() const
	{
		return holdout_woody_point_count_;
	}
	std::size_t excludedTrainingOutlierCount() const
	{
		return excluded_training_outlier_count_;
	}
	std::size_t radiusStationCount() const { return radius_station_count_; }
	std::size_t cylinderCount() const { return cylinder_count_; }
	double woodyLabelFraction() const { return woody_label_fraction_; }
	double excludedTrainingOutlierFraction() const
	{
		return excluded_training_outlier_fraction_;
	}
	double principalVarianceFraction() const
	{
		return principal_variance_fraction_;
	}
	double trainingSurfaceRmseMetres() const
	{
		return training_surface_rmse_metres_;
	}
	double holdoutSurfaceRmseMetres() const
	{
		return holdout_surface_rmse_metres_;
	}
	double holdoutAxialCoverageFraction() const
	{
		return holdout_axial_coverage_fraction_;
	}
	double taperViolationFraction() const
	{
		return taper_violation_fraction_;
	}
	const std::vector<std::string> &rejectionReasons() const
	{
		return rejection_reasons_;
	}

private:
	std::string reconstruction_identifier_;
	std::string dataset_identifier_;
	std::string source_payload_sha256_;
	std::size_t training_woody_point_count_ = 0u;
	std::size_t holdout_woody_point_count_ = 0u;
	std::size_t excluded_training_outlier_count_ = 0u;
	std::size_t radius_station_count_ = 0u;
	std::size_t cylinder_count_ = 0u;
	double woody_label_fraction_ = 0.0;
	double excluded_training_outlier_fraction_ = 0.0;
	double principal_variance_fraction_ = 0.0;
	double training_surface_rmse_metres_ = 0.0;
	double holdout_surface_rmse_metres_ = 0.0;
	double holdout_axial_coverage_fraction_ = 0.0;
	double taper_violation_fraction_ = 0.0;
	std::vector<std::string> rejection_reasons_;
};
