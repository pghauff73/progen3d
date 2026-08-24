#pragma once

#include <cstddef>

class VegetationComplexityLimits
{
public:
	VegetationComplexityLimits(
		std::size_t maximum_branch_nodes = 100000,
		std::size_t maximum_branch_segments = 99999,
		std::size_t maximum_buds = 250000,
		std::size_t maximum_organ_attachments = 250000,
		std::size_t maximum_growth_tips = 100000,
		std::size_t maximum_growth_iterations = 4096,
		std::size_t maximum_attraction_points = 250000,
		std::size_t maximum_scatter_placements = 250000,
		std::size_t maximum_l_system_symbols = 1000000)
		: maximum_branch_nodes_(maximum_branch_nodes),
		  maximum_branch_segments_(maximum_branch_segments),
		  maximum_buds_(maximum_buds),
		  maximum_organ_attachments_(maximum_organ_attachments),
		  maximum_growth_tips_(maximum_growth_tips),
		  maximum_growth_iterations_(maximum_growth_iterations),
		  maximum_attraction_points_(maximum_attraction_points),
		  maximum_scatter_placements_(maximum_scatter_placements),
		  maximum_l_system_symbols_(maximum_l_system_symbols)
	{
	}

	std::size_t maximumBranchNodes() const { return maximum_branch_nodes_; }
	std::size_t maximumBranchSegments() const
	{
		return maximum_branch_segments_;
	}
	std::size_t maximumBuds() const { return maximum_buds_; }
	std::size_t maximumOrganAttachments() const
	{
		return maximum_organ_attachments_;
	}
	std::size_t maximumGrowthTips() const { return maximum_growth_tips_; }
	std::size_t maximumGrowthIterations() const
	{
		return maximum_growth_iterations_;
	}
	std::size_t maximumAttractionPoints() const
	{
		return maximum_attraction_points_;
	}
	std::size_t maximumScatterPlacements() const
	{
		return maximum_scatter_placements_;
	}
	std::size_t maximumLSystemSymbols() const
	{
		return maximum_l_system_symbols_;
	}

private:
	std::size_t maximum_branch_nodes_ = 100000;
	std::size_t maximum_branch_segments_ = 99999;
	std::size_t maximum_buds_ = 250000;
	std::size_t maximum_organ_attachments_ = 250000;
	std::size_t maximum_growth_tips_ = 100000;
	std::size_t maximum_growth_iterations_ = 4096;
	std::size_t maximum_attraction_points_ = 250000;
	std::size_t maximum_scatter_placements_ = 250000;
	std::size_t maximum_l_system_symbols_ = 1000000;
};
