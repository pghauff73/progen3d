#pragma once

#include <cstddef>

class VegetationWoodySegmentationSensitivityPolicy
{
public:
	VegetationWoodySegmentationSensitivityPolicy(
		std::size_t minimum_candidate_count,
		std::size_t minimum_accepted_candidate_count,
		double minimum_accepted_candidate_fraction,
		double minimum_selected_assignment_agreement_fraction,
		double minimum_axis_count_consensus_fraction,
		double minimum_branch_order_consensus_fraction,
		double maximum_cylinder_count_coefficient_of_variation,
		double maximum_axis_length_coefficient_of_variation,
		double maximum_woody_volume_coefficient_of_variation)
		: minimum_candidate_count_(minimum_candidate_count),
		  minimum_accepted_candidate_count_(minimum_accepted_candidate_count),
		  minimum_accepted_candidate_fraction_(
			  minimum_accepted_candidate_fraction),
		  minimum_selected_assignment_agreement_fraction_(
			  minimum_selected_assignment_agreement_fraction),
		  minimum_axis_count_consensus_fraction_(
			  minimum_axis_count_consensus_fraction),
		  minimum_branch_order_consensus_fraction_(
			  minimum_branch_order_consensus_fraction),
		  maximum_cylinder_count_coefficient_of_variation_(
			  maximum_cylinder_count_coefficient_of_variation),
		  maximum_axis_length_coefficient_of_variation_(
			  maximum_axis_length_coefficient_of_variation),
		  maximum_woody_volume_coefficient_of_variation_(
			  maximum_woody_volume_coefficient_of_variation)
	{
	}

	std::size_t minimumCandidateCount() const
	{
		return minimum_candidate_count_;
	}
	std::size_t minimumAcceptedCandidateCount() const
	{
		return minimum_accepted_candidate_count_;
	}
	double minimumAcceptedCandidateFraction() const
	{
		return minimum_accepted_candidate_fraction_;
	}
	double minimumSelectedAssignmentAgreementFraction() const
	{
		return minimum_selected_assignment_agreement_fraction_;
	}
	double minimumAxisCountConsensusFraction() const
	{
		return minimum_axis_count_consensus_fraction_;
	}
	double minimumBranchOrderConsensusFraction() const
	{
		return minimum_branch_order_consensus_fraction_;
	}
	double maximumCylinderCountCoefficientOfVariation() const
	{
		return maximum_cylinder_count_coefficient_of_variation_;
	}
	double maximumAxisLengthCoefficientOfVariation() const
	{
		return maximum_axis_length_coefficient_of_variation_;
	}
	double maximumWoodyVolumeCoefficientOfVariation() const
	{
		return maximum_woody_volume_coefficient_of_variation_;
	}

private:
	std::size_t minimum_candidate_count_ = 0u;
	std::size_t minimum_accepted_candidate_count_ = 0u;
	double minimum_accepted_candidate_fraction_ = 0.0;
	double minimum_selected_assignment_agreement_fraction_ = 0.0;
	double minimum_axis_count_consensus_fraction_ = 0.0;
	double minimum_branch_order_consensus_fraction_ = 0.0;
	double maximum_cylinder_count_coefficient_of_variation_ = 0.0;
	double maximum_axis_length_coefficient_of_variation_ = 0.0;
	double maximum_woody_volume_coefficient_of_variation_ = 0.0;
};
