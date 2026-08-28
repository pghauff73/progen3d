#pragma once

#include "vegetation/model/ScatterPlacement.h"
#include "vegetation/model/ScatterRegionResolutionEvidence.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

class ScatterRegionSnapshot
{
public:
	ScatterRegionSnapshot(
		std::vector<ScatterPlacement> placements,
		ScatterRegionResolutionEvidence evidence)
		: placements_(std::move(placements)), evidence_(std::move(evidence))
	{
	}

	const std::vector<ScatterPlacement> &placements() const
	{
		return placements_;
	}
	const ScatterRegionResolutionEvidence &evidence() const
	{
		return evidence_;
	}

private:
	std::vector<ScatterPlacement> placements_;
	ScatterRegionResolutionEvidence evidence_;
};

class ScatterRegionResult
{
public:
	static ScatterRegionResult succeeded(ScatterRegionSnapshot snapshot)
	{
		return ScatterRegionResult(std::move(snapshot), {});
	}

	static ScatterRegionResult failed(std::string diagnostic)
	{
		return ScatterRegionResult(std::nullopt, std::move(diagnostic));
	}

	bool succeeded() const { return snapshot_.has_value(); }
	const std::optional<ScatterRegionSnapshot> &snapshot() const
	{
		return snapshot_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	ScatterRegionResult(
		std::optional<ScatterRegionSnapshot> snapshot,
		std::string diagnostic)
		: snapshot_(std::move(snapshot)), diagnostic_(std::move(diagnostic))
	{
	}

	std::optional<ScatterRegionSnapshot> snapshot_;
	std::string diagnostic_;
};
