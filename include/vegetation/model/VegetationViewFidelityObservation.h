#pragma once

#include "vegetation/model/VegetationReferenceView.h"

class VegetationViewFidelityObservation
{
public:
	VegetationViewFidelityObservation(
		VegetationReferenceView view,
		double silhouette_match_percent,
		double projected_foliage_area_match_percent,
		double crown_gap_fraction_match_percent,
		double branch_topology_match_percent,
		double material_area_match_percent,
		double motion_envelope_match_percent)
		: view_(view),
		  silhouette_match_percent_(silhouette_match_percent),
		  projected_foliage_area_match_percent_(
			  projected_foliage_area_match_percent),
		  crown_gap_fraction_match_percent_(crown_gap_fraction_match_percent),
		  branch_topology_match_percent_(branch_topology_match_percent),
		  material_area_match_percent_(material_area_match_percent),
		  motion_envelope_match_percent_(motion_envelope_match_percent)
	{
	}

	VegetationReferenceView view() const { return view_; }
	double silhouetteMatchPercent() const { return silhouette_match_percent_; }
	double projectedFoliageAreaMatchPercent() const
	{
		return projected_foliage_area_match_percent_;
	}
	double crownGapFractionMatchPercent() const
	{
		return crown_gap_fraction_match_percent_;
	}
	double branchTopologyMatchPercent() const
	{
		return branch_topology_match_percent_;
	}
	double materialAreaMatchPercent() const
	{
		return material_area_match_percent_;
	}
	double motionEnvelopeMatchPercent() const
	{
		return motion_envelope_match_percent_;
	}

private:
	VegetationReferenceView view_ = VegetationReferenceView::Front;
	double silhouette_match_percent_ = 0.0;
	double projected_foliage_area_match_percent_ = 0.0;
	double crown_gap_fraction_match_percent_ = 0.0;
	double branch_topology_match_percent_ = 0.0;
	double material_area_match_percent_ = 0.0;
	double motion_envelope_match_percent_ = 0.0;
};
