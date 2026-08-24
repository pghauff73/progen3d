#pragma once

#include "vegetation/model/BranchGraph.h"
#include "vegetation/model/SpaceColonizationResolutionEvidence.h"

#include <optional>
#include <string>
#include <utility>

class SpaceColonizationSnapshot
{
public:
	SpaceColonizationSnapshot(
		BranchGraph graph,
		SpaceColonizationResolutionEvidence evidence)
		: graph_(std::move(graph)), evidence_(std::move(evidence))
	{
	}

	const BranchGraph &graph() const { return graph_; }
	const SpaceColonizationResolutionEvidence &evidence() const
	{
		return evidence_;
	}

private:
	BranchGraph graph_;
	SpaceColonizationResolutionEvidence evidence_;
};

class SpaceColonizationResult
{
public:
	static SpaceColonizationResult succeeded(
		SpaceColonizationSnapshot snapshot)
	{
		return SpaceColonizationResult(std::move(snapshot), {});
	}

	static SpaceColonizationResult failed(std::string diagnostic)
	{
		return SpaceColonizationResult(std::nullopt, std::move(diagnostic));
	}

	bool succeeded() const { return snapshot_.has_value(); }
	const std::optional<SpaceColonizationSnapshot> &snapshot() const
	{
		return snapshot_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	SpaceColonizationResult(
		std::optional<SpaceColonizationSnapshot> snapshot,
		std::string diagnostic)
		: snapshot_(std::move(snapshot)), diagnostic_(std::move(diagnostic))
	{
	}

	std::optional<SpaceColonizationSnapshot> snapshot_;
	std::string diagnostic_;
};
