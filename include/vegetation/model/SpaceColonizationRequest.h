#pragma once

#include "vegetation/model/BranchGraph.h"
#include "vegetation/model/CrownVolumeSpecification.h"
#include "vegetation/model/SpaceColonizationSpecification.h"
#include "vegetation/model/VegetationObstacleBoundary.h"

#include <cstdint>
#include <utility>
#include <vector>

class SpaceColonizationRequest
{
public:
	SpaceColonizationRequest(
		BranchGraph source_graph,
		CrownVolumeSpecification crown_volume,
		SpaceColonizationSpecification specification,
		std::uint64_t deterministic_seed,
		std::vector<VegetationObstacleBoundary> obstacles = {})
		: source_graph_(std::move(source_graph)),
		  crown_volume_(std::move(crown_volume)),
		  specification_(std::move(specification)),
		  deterministic_seed_(deterministic_seed),
		  obstacles_(std::move(obstacles))
	{
	}

	const BranchGraph &sourceGraph() const { return source_graph_; }
	const CrownVolumeSpecification &crownVolume() const
	{
		return crown_volume_;
	}
	const SpaceColonizationSpecification &specification() const
	{
		return specification_;
	}
	std::uint64_t deterministicSeed() const { return deterministic_seed_; }
	const std::vector<VegetationObstacleBoundary> &obstacles() const
	{
		return obstacles_;
	}

private:
	BranchGraph source_graph_;
	CrownVolumeSpecification crown_volume_;
	SpaceColonizationSpecification specification_;
	std::uint64_t deterministic_seed_ = 0;
	std::vector<VegetationObstacleBoundary> obstacles_;
};
