#pragma once

#include "vegetation/model/VegetationWoodyBranchAxis.h"
#include "vegetation/model/VegetationWoodyBranchConnection.h"
#include "vegetation/model/VegetationWoodyPointSegmentationValidationReport.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class VegetationWoodyBranchAxisQualityEvidence
{
public:
	VegetationWoodyBranchAxisQualityEvidence(
		std::string axis_identifier,
		std::size_t training_point_count,
		std::size_t holdout_point_count,
		double principal_variance_fraction,
		double training_surface_rmse_metres,
		double holdout_surface_rmse_metres,
		double holdout_surface_coverage_fraction,
		double taper_violation_fraction)
		: axis_identifier_(std::move(axis_identifier)),
		  training_point_count_(training_point_count),
		  holdout_point_count_(holdout_point_count),
		  principal_variance_fraction_(principal_variance_fraction),
		  training_surface_rmse_metres_(training_surface_rmse_metres),
		  holdout_surface_rmse_metres_(holdout_surface_rmse_metres),
		  holdout_surface_coverage_fraction_(
			  holdout_surface_coverage_fraction),
		  taper_violation_fraction_(taper_violation_fraction)
	{
	}

	const std::string &axisIdentifier() const { return axis_identifier_; }
	std::size_t trainingPointCount() const { return training_point_count_; }
	std::size_t holdoutPointCount() const { return holdout_point_count_; }
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
	double holdoutSurfaceCoverageFraction() const
	{
		return holdout_surface_coverage_fraction_;
	}
	double taperViolationFraction() const
	{
		return taper_violation_fraction_;
	}

private:
	std::string axis_identifier_;
	std::size_t training_point_count_ = 0u;
	std::size_t holdout_point_count_ = 0u;
	double principal_variance_fraction_ = 0.0;
	double training_surface_rmse_metres_ = 0.0;
	double holdout_surface_rmse_metres_ = 0.0;
	double holdout_surface_coverage_fraction_ = 0.0;
	double taper_violation_fraction_ = 0.0;
};

class VegetationWoodyBranchGraphQualityEvidence
{
public:
	VegetationWoodyBranchGraphQualityEvidence(
		std::string reconstruction_identifier,
		std::string dataset_identifier,
		std::string source_payload_sha256,
		VegetationWoodyPointSegmentationValidationReport segmentation_report,
		std::vector<VegetationWoodyBranchAxis> axes,
		std::vector<VegetationWoodyBranchConnection> connections,
		std::vector<VegetationWoodyBranchAxisQualityEvidence> axis_quality)
		: reconstruction_identifier_(std::move(reconstruction_identifier)),
		  dataset_identifier_(std::move(dataset_identifier)),
		  source_payload_sha256_(std::move(source_payload_sha256)),
		  segmentation_report_(std::move(segmentation_report)),
		  axes_(std::move(axes)),
		  connections_(std::move(connections)),
		  axis_quality_(std::move(axis_quality))
	{
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
	const VegetationWoodyPointSegmentationValidationReport &segmentationReport()
		const
	{
		return segmentation_report_;
	}
	const std::vector<VegetationWoodyBranchAxis> &axes() const { return axes_; }
	const std::vector<VegetationWoodyBranchConnection> &connections() const
	{
		return connections_;
	}
	const std::vector<VegetationWoodyBranchAxisQualityEvidence> &axisQuality()
		const
	{
		return axis_quality_;
	}

private:
	std::string reconstruction_identifier_;
	std::string dataset_identifier_;
	std::string source_payload_sha256_;
	VegetationWoodyPointSegmentationValidationReport segmentation_report_{
		0u, 0u, 0u, 0.0, {}};
	std::vector<VegetationWoodyBranchAxis> axes_;
	std::vector<VegetationWoodyBranchConnection> connections_;
	std::vector<VegetationWoodyBranchAxisQualityEvidence> axis_quality_;
};
