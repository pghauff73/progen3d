#pragma once

#include "vegetation/model/BranchGraph.h"
#include "vegetation/model/PlantGrowthResolutionEvidence.h"

#include <optional>
#include <string>
#include <utility>

class PlantGrowthSnapshot
{
public:
	PlantGrowthSnapshot(
		BranchGraph graph,
		PlantGrowthResolutionEvidence evidence)
		: graph_(std::move(graph)), evidence_(std::move(evidence))
	{
	}

	const BranchGraph &graph() const { return graph_; }
	const PlantGrowthResolutionEvidence &evidence() const { return evidence_; }

private:
	BranchGraph graph_;
	PlantGrowthResolutionEvidence evidence_;
};

class PlantGrowthResult
{
public:
	static PlantGrowthResult succeeded(PlantGrowthSnapshot snapshot)
	{
		return PlantGrowthResult(std::move(snapshot), {});
	}

	static PlantGrowthResult failed(std::string diagnostic)
	{
		return PlantGrowthResult(std::nullopt, std::move(diagnostic));
	}

	bool succeeded() const { return snapshot_.has_value(); }
	const std::optional<PlantGrowthSnapshot> &snapshot() const
	{
		return snapshot_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	PlantGrowthResult(
		std::optional<PlantGrowthSnapshot> snapshot,
		std::string diagnostic)
		: snapshot_(std::move(snapshot)), diagnostic_(std::move(diagnostic))
	{
	}

	std::optional<PlantGrowthSnapshot> snapshot_;
	std::string diagnostic_;
};
