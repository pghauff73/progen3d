#pragma once

class PlantBranchingSpecification
{
public:
	PlantBranchingSpecification(
		float primary_axis_length,
		float base_radius,
		float tip_radius,
		int primary_internode_count,
		int maximum_branch_order,
		int lateral_branches_per_node,
		int branch_every_n_internodes,
		float branch_angle_degrees,
		float azimuth_divergence_degrees,
		float branch_length_fraction,
		float branch_length_falloff,
		float continuation_radius_ratio,
		float radius_conservation_gamma,
		float direction_variation_degrees,
		float length_variation_fraction,
		float upward_tropism_weight)
		: primary_axis_length_(primary_axis_length),
		  base_radius_(base_radius),
		  tip_radius_(tip_radius),
		  primary_internode_count_(primary_internode_count),
		  maximum_branch_order_(maximum_branch_order),
		  lateral_branches_per_node_(lateral_branches_per_node),
		  branch_every_n_internodes_(branch_every_n_internodes),
		  branch_angle_degrees_(branch_angle_degrees),
		  azimuth_divergence_degrees_(azimuth_divergence_degrees),
		  branch_length_fraction_(branch_length_fraction),
		  branch_length_falloff_(branch_length_falloff),
		  continuation_radius_ratio_(continuation_radius_ratio),
		  radius_conservation_gamma_(radius_conservation_gamma),
		  direction_variation_degrees_(direction_variation_degrees),
		  length_variation_fraction_(length_variation_fraction),
		  upward_tropism_weight_(upward_tropism_weight)
	{
	}

	float primaryAxisLength() const { return primary_axis_length_; }
	float baseRadius() const { return base_radius_; }
	float tipRadius() const { return tip_radius_; }
	int primaryInternodeCount() const { return primary_internode_count_; }
	int maximumBranchOrder() const { return maximum_branch_order_; }
	int lateralBranchesPerNode() const { return lateral_branches_per_node_; }
	int branchEveryNInternodes() const { return branch_every_n_internodes_; }
	float branchAngleDegrees() const { return branch_angle_degrees_; }
	float azimuthDivergenceDegrees() const
	{
		return azimuth_divergence_degrees_;
	}
	float branchLengthFraction() const { return branch_length_fraction_; }
	float branchLengthFalloff() const { return branch_length_falloff_; }
	float continuationRadiusRatio() const
	{
		return continuation_radius_ratio_;
	}
	float radiusConservationGamma() const
	{
		return radius_conservation_gamma_;
	}
	float directionVariationDegrees() const
	{
		return direction_variation_degrees_;
	}
	float lengthVariationFraction() const
	{
		return length_variation_fraction_;
	}
	float upwardTropismWeight() const { return upward_tropism_weight_; }

private:
	float primary_axis_length_ = 1.0f;
	float base_radius_ = 0.1f;
	float tip_radius_ = 0.01f;
	int primary_internode_count_ = 6;
	int maximum_branch_order_ = 2;
	int lateral_branches_per_node_ = 1;
	int branch_every_n_internodes_ = 2;
	float branch_angle_degrees_ = 42.0f;
	float azimuth_divergence_degrees_ = 137.5f;
	float branch_length_fraction_ = 0.42f;
	float branch_length_falloff_ = 0.62f;
	float continuation_radius_ratio_ = 0.88f;
	float radius_conservation_gamma_ = 2.0f;
	float direction_variation_degrees_ = 4.0f;
	float length_variation_fraction_ = 0.08f;
	float upward_tropism_weight_ = 0.15f;
};

