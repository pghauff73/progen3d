#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class VegetationWoodySegmentationSensitivityReport
{
public:
	VegetationWoodySegmentationSensitivityReport(
		std::size_t candidate_count,
		std::size_t accepted_candidate_count,
		std::optional<std::size_t> selected_candidate_index,
		std::string selected_parameter_identifier,
		double accepted_candidate_fraction,
		double minimum_selected_assignment_agreement_fraction,
		double axis_count_consensus_fraction,
		double branch_order_consensus_fraction,
		double cylinder_count_coefficient_of_variation,
		double axis_length_coefficient_of_variation,
		double woody_volume_coefficient_of_variation,
		std::vector<std::string> rejection_reasons)
		: candidate_count_(candidate_count),
		  accepted_candidate_count_(accepted_candidate_count),
		  selected_candidate_index_(selected_candidate_index),
		  selected_parameter_identifier_(
			  std::move(selected_parameter_identifier)),
		  accepted_candidate_fraction_(accepted_candidate_fraction),
		  minimum_selected_assignment_agreement_fraction_(
			  minimum_selected_assignment_agreement_fraction),
		  axis_count_consensus_fraction_(axis_count_consensus_fraction),
		  branch_order_consensus_fraction_(branch_order_consensus_fraction),
		  cylinder_count_coefficient_of_variation_(
			  cylinder_count_coefficient_of_variation),
		  axis_length_coefficient_of_variation_(
			  axis_length_coefficient_of_variation),
		  woody_volume_coefficient_of_variation_(
			  woody_volume_coefficient_of_variation),
		  rejection_reasons_(std::move(rejection_reasons))
	{
	}

	bool acceptedForGraphSelection() const
	{
		return rejection_reasons_.empty() && selected_candidate_index_.has_value();
	}
	std::size_t candidateCount() const { return candidate_count_; }
	std::size_t acceptedCandidateCount() const
	{
		return accepted_candidate_count_;
	}
	const std::optional<std::size_t> &selectedCandidateIndex() const
	{
		return selected_candidate_index_;
	}
	const std::string &selectedParameterIdentifier() const
	{
		return selected_parameter_identifier_;
	}
	double acceptedCandidateFraction() const
	{
		return accepted_candidate_fraction_;
	}
	double minimumSelectedAssignmentAgreementFraction() const
	{
		return minimum_selected_assignment_agreement_fraction_;
	}
	double axisCountConsensusFraction() const
	{
		return axis_count_consensus_fraction_;
	}
	double branchOrderConsensusFraction() const
	{
		return branch_order_consensus_fraction_;
	}
	double cylinderCountCoefficientOfVariation() const
	{
		return cylinder_count_coefficient_of_variation_;
	}
	double axisLengthCoefficientOfVariation() const
	{
		return axis_length_coefficient_of_variation_;
	}
	double woodyVolumeCoefficientOfVariation() const
	{
		return woody_volume_coefficient_of_variation_;
	}
	const std::vector<std::string> &rejectionReasons() const
	{
		return rejection_reasons_;
	}

private:
	std::size_t candidate_count_ = 0u;
	std::size_t accepted_candidate_count_ = 0u;
	std::optional<std::size_t> selected_candidate_index_;
	std::string selected_parameter_identifier_;
	double accepted_candidate_fraction_ = 0.0;
	double minimum_selected_assignment_agreement_fraction_ = 0.0;
	double axis_count_consensus_fraction_ = 0.0;
	double branch_order_consensus_fraction_ = 0.0;
	double cylinder_count_coefficient_of_variation_ = 0.0;
	double axis_length_coefficient_of_variation_ = 0.0;
	double woody_volume_coefficient_of_variation_ = 0.0;
	std::vector<std::string> rejection_reasons_;
};
