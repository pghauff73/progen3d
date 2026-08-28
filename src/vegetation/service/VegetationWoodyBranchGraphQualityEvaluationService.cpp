#include "vegetation/service/VegetationWoodyBranchGraphQualityEvaluationService.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <string>
#include <utility>
#include <vector>

VegetationWoodyBranchGraphQualityReport
VegetationWoodyBranchGraphQualityEvaluationService::evaluate(
	const VegetationWoodyBranchGraphQualityEvidence &evidence,
	const VegetationWoodyBranchGraphReconstructionPolicy &policy) const
{
	const auto &segmentation = evidence.segmentationReport();
	const double assignment_coverage = segmentation.woodyPointCount() == 0u
		                                   ? 0.0
		                                   : static_cast<double>(
			                                     segmentation
				                                     .assignedWoodyPointCount()) /
			                                     segmentation.woodyPointCount();
	std::size_t cylinder_count = 0u;
	std::size_t maximum_branch_order = 0u;
	for (const auto &axis : evidence.axes()) {
		cylinder_count += axis.cylinders().size();
		maximum_branch_order =
			std::max(maximum_branch_order, axis.branchOrder());
	}
	double minimum_principal_variance = 1.0;
	double maximum_training_rmse = 0.0;
	double maximum_holdout_rmse = 0.0;
	double minimum_holdout_coverage = 1.0;
	double maximum_taper_violation = 0.0;
	for (const auto &axis : evidence.axisQuality()) {
		minimum_principal_variance = std::min(
			minimum_principal_variance, axis.principalVarianceFraction());
		maximum_training_rmse = std::max(
			maximum_training_rmse, axis.trainingSurfaceRmseMetres());
		maximum_holdout_rmse = std::max(
			maximum_holdout_rmse, axis.holdoutSurfaceRmseMetres());
		minimum_holdout_coverage = std::min(
			minimum_holdout_coverage,
			axis.holdoutSurfaceCoverageFraction());
		maximum_taper_violation = std::max(
			maximum_taper_violation, axis.taperViolationFraction());
	}
	if (evidence.axisQuality().empty()) {
		minimum_principal_variance = 0.0;
		minimum_holdout_coverage = 0.0;
	}
	double maximum_attachment_gap = 0.0;
	for (const auto &connection : evidence.connections()) {
		maximum_attachment_gap = std::max(
			maximum_attachment_gap,
			connection.attachmentSurfaceGapMetres());
	}
	const double unassigned_fraction = 1.0 - assignment_coverage;
	std::vector<std::string> rejection_reasons;
	if (!segmentation.acceptedForGraphConstruction()) {
		rejection_reasons.emplace_back(
			"Woody point segmentation validation is rejected.");
	}
	if (evidence.axes().empty() || evidence.axisQuality().size() !=
		                             evidence.axes().size()) {
		rejection_reasons.emplace_back(
			"Woody branch graph has incomplete axis quality evidence.");
	}
	if (evidence.axes().size() > 1u &&
	    evidence.connections().size() + 1u != evidence.axes().size()) {
		rejection_reasons.emplace_back(
			"Woody branch graph connection count does not form a rooted tree.");
	}
	if (maximum_branch_order == 0u) {
		rejection_reasons.emplace_back(
			"Woody branch graph contains no lateral branch order.");
	}
	if (unassigned_fraction > policy.maximumUnassignedWoodyFraction()) {
		rejection_reasons.emplace_back(
			"Unassigned woody point fraction exceeds the graph boundary.");
	}
	if (segmentation.minimumAssignmentConfidence() <
	    policy.minimumAssignmentConfidence()) {
		rejection_reasons.emplace_back(
			"Minimum point-segment assignment confidence is too low.");
	}
	if (minimum_principal_variance <
	    policy.minimumPrincipalVarianceFraction()) {
		rejection_reasons.emplace_back(
			"An axis principal variance fraction is below the graph boundary.");
	}
	if (maximum_training_rmse >
	    policy.maximumTrainingSurfaceRmseMetres()) {
		rejection_reasons.emplace_back(
			"An axis training surface RMSE exceeds the graph boundary.");
	}
	if (maximum_holdout_rmse > policy.maximumHoldoutSurfaceRmseMetres()) {
		rejection_reasons.emplace_back(
			"An axis holdout surface RMSE exceeds the graph boundary.");
	}
	if (minimum_holdout_coverage <
	    policy.minimumHoldoutSurfaceCoverageFraction()) {
		rejection_reasons.emplace_back(
			"An axis holdout surface coverage is below the graph boundary.");
	}
	if (maximum_attachment_gap >
	    policy.maximumAttachmentSurfaceGapMetres()) {
		rejection_reasons.emplace_back(
			"A child axis attachment surface gap exceeds the graph boundary.");
	}
	if (maximum_taper_violation >
	    policy.maximumTaperViolationFraction()) {
		rejection_reasons.emplace_back(
			"An axis taper violation fraction exceeds the graph boundary.");
	}
	return VegetationWoodyBranchGraphQualityReport(
		evidence.reconstructionIdentifier(), evidence.datasetIdentifier(),
		evidence.sourcePayloadSha256(), evidence.axes().size(), cylinder_count,
		evidence.connections().size(), maximum_branch_order,
		assignment_coverage, segmentation.minimumAssignmentConfidence(),
		minimum_principal_variance, maximum_training_rmse,
		maximum_holdout_rmse, minimum_holdout_coverage,
		maximum_attachment_gap, maximum_taper_violation,
		std::move(rejection_reasons));
}
