#include "vegetation/service/VegetationWoodySegmentationSensitivityEvaluationService.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

bool fraction(double value)
{
	return std::isfinite(value) && value >= 0.0 && value <= 1.0;
}

bool non_negative(double value)
{
	return std::isfinite(value) && value >= 0.0;
}

bool valid_policy(const VegetationWoodySegmentationSensitivityPolicy &policy)
{
	return policy.minimumCandidateCount() > 0u &&
	       policy.minimumAcceptedCandidateCount() > 0u &&
	       policy.minimumAcceptedCandidateCount() <=
		       policy.minimumCandidateCount() &&
	       fraction(policy.minimumAcceptedCandidateFraction()) &&
	       fraction(
		       policy.minimumSelectedAssignmentAgreementFraction()) &&
	       fraction(policy.minimumAxisCountConsensusFraction()) &&
	       fraction(policy.minimumBranchOrderConsensusFraction()) &&
	       non_negative(
		       policy.maximumCylinderCountCoefficientOfVariation()) &&
	       non_negative(policy.maximumAxisLengthCoefficientOfVariation()) &&
	       non_negative(policy.maximumWoodyVolumeCoefficientOfVariation());
}

std::map<std::size_t, std::string> assignments(
	const VegetationWoodySegmentationCandidateEvaluation &evaluation)
{
	std::map<std::size_t, std::string> result;
	if (!evaluation.segmentationReport().candidate().has_value()) return result;
	for (const auto &assignment : evaluation.segmentationReport()
	                                  .candidate()
	                                  ->segmentation()
	                                  .pointAssignments()) {
		result.emplace(
			assignment.sourcePointIndex(), assignment.axisIdentifier());
	}
	return result;
}

double assignment_agreement(
	const VegetationWoodySegmentationCandidateEvaluation &first,
	const VegetationWoodySegmentationCandidateEvaluation &second)
{
	const auto first_assignments = assignments(first);
	const auto second_assignments = assignments(second);
	std::set<std::size_t> source_indices;
	for (const auto &entry : first_assignments) source_indices.insert(entry.first);
	for (const auto &entry : second_assignments) source_indices.insert(entry.first);
	if (source_indices.empty()) return 0.0;
	std::size_t agreement_count = 0u;
	for (const std::size_t source_index : source_indices) {
		const auto first_assignment = first_assignments.find(source_index);
		const auto second_assignment = second_assignments.find(source_index);
		if (first_assignment != first_assignments.end() &&
		    second_assignment != second_assignments.end() &&
		    first_assignment->second == second_assignment->second) {
			++agreement_count;
		}
	}
	return static_cast<double>(agreement_count) /
	       static_cast<double>(source_indices.size());
}

double coefficient_of_variation(const std::vector<double> &values)
{
	if (values.empty()) return std::numeric_limits<double>::infinity();
	const double mean = std::accumulate(values.begin(), values.end(), 0.0) /
	                    static_cast<double>(values.size());
	if (!std::isfinite(mean) || mean <= 0.0) {
		return std::numeric_limits<double>::infinity();
	}
	double squared_deviation_sum = 0.0;
	for (const double value : values) {
		const double deviation = value - mean;
		squared_deviation_sum += deviation * deviation;
	}
	const double standard_deviation = std::sqrt(
		squared_deviation_sum / static_cast<double>(values.size()));
	return standard_deviation / mean;
}

const VegetationWoodyBranchGraphReconstruction &reconstruction(
	const VegetationWoodySegmentationCandidateEvaluation &evaluation)
{
	return *evaluation.graphReport()->reconstruction();
}

}

VegetationWoodySegmentationSensitivityReport
VegetationWoodySegmentationSensitivityEvaluationService::evaluate(
	const std::vector<VegetationWoodySegmentationCandidateEvaluation> &evaluations,
	const VegetationWoodySegmentationSensitivityPolicy &policy) const
{
	std::vector<std::string> rejection_reasons;
	if (!valid_policy(policy)) {
		rejection_reasons.emplace_back(
			"Woody segmentation sensitivity policy is invalid.");
	}
	if (evaluations.size() < policy.minimumCandidateCount()) {
		rejection_reasons.emplace_back(
			"Woody segmentation ensemble has too few parameter candidates.");
	}
	std::vector<std::size_t> accepted_indices;
	for (std::size_t index = 0u; index < evaluations.size(); ++index) {
		if (evaluations[index].acceptedForSensitivity()) {
			accepted_indices.push_back(index);
		}
	}
	const double accepted_fraction = evaluations.empty()
		                                 ? 0.0
		                                 : static_cast<double>(
			                                 accepted_indices.size()) /
			                                 static_cast<double>(
				                                 evaluations.size());
	if (accepted_indices.size() < policy.minimumAcceptedCandidateCount()) {
		rejection_reasons.emplace_back(
			"Too few woody segmentation candidates passed graph reconstruction.");
	}
	if (accepted_fraction < policy.minimumAcceptedCandidateFraction()) {
		rejection_reasons.emplace_back(
			"Accepted woody segmentation candidate fraction is too low.");
	}

	std::optional<std::size_t> selected_index;
	double selected_mean_agreement = -1.0;
	double selected_holdout_rmse = std::numeric_limits<double>::infinity();
	for (const std::size_t candidate_index : accepted_indices) {
		double agreement_sum = 0.0;
		for (const std::size_t other_index : accepted_indices) {
			agreement_sum += assignment_agreement(
				evaluations[candidate_index], evaluations[other_index]);
		}
		const double mean_agreement = agreement_sum /
		                              static_cast<double>(
			                              accepted_indices.size());
		const double holdout_rmse = reconstruction(evaluations[candidate_index])
		                                .qualityReport()
		                                .maximumHoldoutSurfaceRmseMetres();
		const bool better_agreement =
			mean_agreement > selected_mean_agreement + 1.0e-12;
		const bool equal_agreement =
			std::abs(mean_agreement - selected_mean_agreement) <= 1.0e-12;
		const bool better_residual =
			holdout_rmse < selected_holdout_rmse - 1.0e-12;
		const bool equal_residual =
			std::abs(holdout_rmse - selected_holdout_rmse) <= 1.0e-12;
		const bool better_identifier =
			!selected_index.has_value() ||
			evaluations[candidate_index].parameterSet().parameterIdentifier() <
				evaluations[*selected_index].parameterSet().parameterIdentifier();
		if (better_agreement ||
		    (equal_agreement &&
		     (better_residual || (equal_residual && better_identifier)))) {
			selected_index = candidate_index;
			selected_mean_agreement = mean_agreement;
			selected_holdout_rmse = holdout_rmse;
		}
	}

	double minimum_selected_agreement = 0.0;
	double axis_count_consensus = 0.0;
	double branch_order_consensus = 0.0;
	double cylinder_count_variation = std::numeric_limits<double>::infinity();
	double axis_length_variation = std::numeric_limits<double>::infinity();
	double woody_volume_variation = std::numeric_limits<double>::infinity();
	std::string selected_parameter_identifier;
	if (selected_index.has_value()) {
		selected_parameter_identifier =
			evaluations[*selected_index].parameterSet().parameterIdentifier();
		minimum_selected_agreement = 1.0;
		const auto &selected_reconstruction =
			reconstruction(evaluations[*selected_index]);
		std::size_t matching_axis_count = 0u;
		std::size_t matching_branch_order_count = 0u;
		std::vector<double> cylinder_counts;
		std::vector<double> axis_lengths;
		std::vector<double> woody_volumes;
		for (const std::size_t candidate_index : accepted_indices) {
			minimum_selected_agreement = std::min(
				minimum_selected_agreement,
				assignment_agreement(
					evaluations[*selected_index], evaluations[candidate_index]));
			const auto &candidate_reconstruction =
				reconstruction(evaluations[candidate_index]);
			if (candidate_reconstruction.axes().size() ==
			    selected_reconstruction.axes().size()) {
				++matching_axis_count;
			}
			if (candidate_reconstruction.qualityReport().maximumBranchOrder() ==
			    selected_reconstruction.qualityReport().maximumBranchOrder()) {
				++matching_branch_order_count;
			}
			cylinder_counts.push_back(static_cast<double>(
				candidate_reconstruction.qualityReport().cylinderCount()));
			axis_lengths.push_back(
				evaluations[candidate_index].totalAxisLengthMetres());
			woody_volumes.push_back(
				evaluations[candidate_index].woodyVolumeCubicMetres());
		}
		axis_count_consensus = static_cast<double>(matching_axis_count) /
		                       static_cast<double>(accepted_indices.size());
		branch_order_consensus =
			static_cast<double>(matching_branch_order_count) /
			static_cast<double>(accepted_indices.size());
		cylinder_count_variation = coefficient_of_variation(cylinder_counts);
		axis_length_variation = coefficient_of_variation(axis_lengths);
		woody_volume_variation = coefficient_of_variation(woody_volumes);
		if (minimum_selected_agreement <
		    policy.minimumSelectedAssignmentAgreementFraction()) {
			rejection_reasons.emplace_back(
				"Selected woody segmentation assignment agreement is unstable.");
		}
		if (axis_count_consensus < policy.minimumAxisCountConsensusFraction()) {
			rejection_reasons.emplace_back(
				"Woody segmentation axis-count consensus is too low.");
		}
		if (branch_order_consensus <
		    policy.minimumBranchOrderConsensusFraction()) {
			rejection_reasons.emplace_back(
				"Woody segmentation branch-order consensus is too low.");
		}
		if (cylinder_count_variation >
		    policy.maximumCylinderCountCoefficientOfVariation()) {
			rejection_reasons.emplace_back(
				"Woody cylinder-count sensitivity exceeds the policy boundary.");
		}
		if (axis_length_variation >
		    policy.maximumAxisLengthCoefficientOfVariation()) {
			rejection_reasons.emplace_back(
				"Woody axis-length sensitivity exceeds the policy boundary.");
		}
		if (woody_volume_variation >
		    policy.maximumWoodyVolumeCoefficientOfVariation()) {
			rejection_reasons.emplace_back(
				"Woody volume sensitivity exceeds the policy boundary.");
		}
	}

	return VegetationWoodySegmentationSensitivityReport(
		evaluations.size(), accepted_indices.size(), selected_index,
		std::move(selected_parameter_identifier), accepted_fraction,
		minimum_selected_agreement, axis_count_consensus,
		branch_order_consensus, cylinder_count_variation,
		axis_length_variation, woody_volume_variation,
		std::move(rejection_reasons));
}
