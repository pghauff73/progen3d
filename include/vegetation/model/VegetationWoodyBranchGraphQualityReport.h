#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class VegetationWoodyBranchGraphQualityReport
{
public:
	VegetationWoodyBranchGraphQualityReport(
		std::string reconstruction_identifier,
		std::string dataset_identifier,
		std::string source_payload_sha256,
		std::size_t axis_count,
		std::size_t cylinder_count,
		std::size_t connection_count,
		std::size_t maximum_branch_order,
		double assignment_coverage_fraction,
		double minimum_assignment_confidence,
		double minimum_principal_variance_fraction,
		double maximum_training_surface_rmse_metres,
		double maximum_holdout_surface_rmse_metres,
		double minimum_holdout_surface_coverage_fraction,
		double maximum_attachment_surface_gap_metres,
		double maximum_taper_violation_fraction,
		std::vector<std::string> rejection_reasons)
		: reconstruction_identifier_(std::move(reconstruction_identifier)),
		  dataset_identifier_(std::move(dataset_identifier)),
		  source_payload_sha256_(std::move(source_payload_sha256)),
		  axis_count_(axis_count),
		  cylinder_count_(cylinder_count),
		  connection_count_(connection_count),
		  maximum_branch_order_(maximum_branch_order),
		  assignment_coverage_fraction_(assignment_coverage_fraction),
		  minimum_assignment_confidence_(minimum_assignment_confidence),
		  minimum_principal_variance_fraction_(
			  minimum_principal_variance_fraction),
		  maximum_training_surface_rmse_metres_(
			  maximum_training_surface_rmse_metres),
		  maximum_holdout_surface_rmse_metres_(
			  maximum_holdout_surface_rmse_metres),
		  minimum_holdout_surface_coverage_fraction_(
			  minimum_holdout_surface_coverage_fraction),
		  maximum_attachment_surface_gap_metres_(
			  maximum_attachment_surface_gap_metres),
		  maximum_taper_violation_fraction_(
			  maximum_taper_violation_fraction),
		  rejection_reasons_(std::move(rejection_reasons))
	{
	}

	bool acceptedForObservedWoodyGraph() const
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
	std::size_t axisCount() const { return axis_count_; }
	std::size_t cylinderCount() const { return cylinder_count_; }
	std::size_t connectionCount() const { return connection_count_; }
	std::size_t maximumBranchOrder() const { return maximum_branch_order_; }
	double assignmentCoverageFraction() const
	{
		return assignment_coverage_fraction_;
	}
	double minimumAssignmentConfidence() const
	{
		return minimum_assignment_confidence_;
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
	const std::vector<std::string> &rejectionReasons() const
	{
		return rejection_reasons_;
	}

private:
	std::string reconstruction_identifier_;
	std::string dataset_identifier_;
	std::string source_payload_sha256_;
	std::size_t axis_count_ = 0u;
	std::size_t cylinder_count_ = 0u;
	std::size_t connection_count_ = 0u;
	std::size_t maximum_branch_order_ = 0u;
	double assignment_coverage_fraction_ = 0.0;
	double minimum_assignment_confidence_ = 0.0;
	double minimum_principal_variance_fraction_ = 0.0;
	double maximum_training_surface_rmse_metres_ = 0.0;
	double maximum_holdout_surface_rmse_metres_ = 0.0;
	double minimum_holdout_surface_coverage_fraction_ = 0.0;
	double maximum_attachment_surface_gap_metres_ = 0.0;
	double maximum_taper_violation_fraction_ = 0.0;
	std::vector<std::string> rejection_reasons_;
};
