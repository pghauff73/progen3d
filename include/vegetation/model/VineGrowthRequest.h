#pragma once

#include "vegetation/model/BranchGraph.h"
#include "vegetation/model/VegetationObstacleBoundary.h"
#include "vegetation/model/VegetationSurfaceTarget.h"
#include "vegetation/model/VineGrowthSpecification.h"

#include <optional>
#include <utility>
#include <vector>

class VineGrowthRequest
{
public:
	VineGrowthRequest(
		BranchGraph source_graph,
		VineGrowthSpecification specification,
		std::optional<VegetationSurfaceTarget> target = std::nullopt,
		std::vector<VegetationObstacleBoundary> obstacles = {})
		: source_graph_(std::move(source_graph)),
		  specification_(std::move(specification)),
		  target_(std::move(target)),
		  obstacles_(std::move(obstacles))
	{
	}

	const BranchGraph &sourceGraph() const { return source_graph_; }
	const VineGrowthSpecification &specification() const
	{
		return specification_;
	}
	const std::optional<VegetationSurfaceTarget> &target() const
	{
		return target_;
	}
	const std::vector<VegetationObstacleBoundary> &obstacles() const
	{
		return obstacles_;
	}

private:
	BranchGraph source_graph_;
	VineGrowthSpecification specification_;
	std::optional<VegetationSurfaceTarget> target_;
	std::vector<VegetationObstacleBoundary> obstacles_;
};
