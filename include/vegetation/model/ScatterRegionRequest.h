#pragma once

#include "vegetation/model/ScatterObstacleBoundary.h"
#include "vegetation/model/ScatterRegionSpecification.h"

#include <cstdint>
#include <utility>
#include <vector>

class ScatterRegionRequest
{
public:
	ScatterRegionRequest(
		ScatterRegionSpecification specification,
		std::uint64_t deterministic_seed,
		std::vector<ScatterObstacleBoundary> obstacles = {})
		: specification_(std::move(specification)),
		  deterministic_seed_(deterministic_seed),
		  obstacles_(std::move(obstacles))
	{
	}

	const ScatterRegionSpecification &specification() const
	{
		return specification_;
	}
	std::uint64_t deterministicSeed() const { return deterministic_seed_; }
	const std::vector<ScatterObstacleBoundary> &obstacles() const
	{
		return obstacles_;
	}

private:
	ScatterRegionSpecification specification_;
	std::uint64_t deterministic_seed_ = 0;
	std::vector<ScatterObstacleBoundary> obstacles_;
};
