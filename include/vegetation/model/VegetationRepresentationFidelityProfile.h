#pragma once

class VegetationRepresentationFidelityProfile
{
public:
	VegetationRepresentationFidelityProfile(
		double minimum_silhouette_match_percent,
		double minimum_projected_foliage_area_match_percent,
		double minimum_crown_gap_fraction_match_percent,
		double minimum_branch_topology_match_percent,
		double minimum_material_area_match_percent,
		double minimum_motion_envelope_match_percent)
		: minimum_silhouette_match_percent_(minimum_silhouette_match_percent),
		  minimum_projected_foliage_area_match_percent_(
			  minimum_projected_foliage_area_match_percent),
		  minimum_crown_gap_fraction_match_percent_(
			  minimum_crown_gap_fraction_match_percent),
		  minimum_branch_topology_match_percent_(
			  minimum_branch_topology_match_percent),
		  minimum_material_area_match_percent_(
			  minimum_material_area_match_percent),
		  minimum_motion_envelope_match_percent_(
			  minimum_motion_envelope_match_percent)
	{
	}

	double minimumSilhouetteMatchPercent() const
	{
		return minimum_silhouette_match_percent_;
	}
	double minimumProjectedFoliageAreaMatchPercent() const
	{
		return minimum_projected_foliage_area_match_percent_;
	}
	double minimumCrownGapFractionMatchPercent() const
	{
		return minimum_crown_gap_fraction_match_percent_;
	}
	double minimumBranchTopologyMatchPercent() const
	{
		return minimum_branch_topology_match_percent_;
	}
	double minimumMaterialAreaMatchPercent() const
	{
		return minimum_material_area_match_percent_;
	}
	double minimumMotionEnvelopeMatchPercent() const
	{
		return minimum_motion_envelope_match_percent_;
	}

private:
	double minimum_silhouette_match_percent_ = 95.0;
	double minimum_projected_foliage_area_match_percent_ = 95.0;
	double minimum_crown_gap_fraction_match_percent_ = 95.0;
	double minimum_branch_topology_match_percent_ = 95.0;
	double minimum_material_area_match_percent_ = 95.0;
	double minimum_motion_envelope_match_percent_ = 95.0;
};
