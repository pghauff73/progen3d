#pragma once

#include "vegetation/model/BranchGraph.h"
#include "vegetation/model/LSystemBranchGraphGenerationEvidence.h"

#include <optional>
#include <string>
#include <utility>

class LSystemBranchGraphGenerationSnapshot
{
public:
	LSystemBranchGraphGenerationSnapshot(
		BranchGraph graph,
		LSystemBranchGraphGenerationEvidence evidence)
		: graph_(std::move(graph)), evidence_(std::move(evidence))
	{
	}

	const BranchGraph &graph() const { return graph_; }
	const LSystemBranchGraphGenerationEvidence &evidence() const
	{
		return evidence_;
	}

private:
	BranchGraph graph_;
	LSystemBranchGraphGenerationEvidence evidence_;
};

class LSystemBranchGraphGenerationResult
{
public:
	static LSystemBranchGraphGenerationResult succeeded(
		LSystemBranchGraphGenerationSnapshot snapshot)
	{
		return LSystemBranchGraphGenerationResult(std::move(snapshot), {});
	}

	static LSystemBranchGraphGenerationResult failed(std::string diagnostic)
	{
		return LSystemBranchGraphGenerationResult(
			std::nullopt, std::move(diagnostic));
	}

	bool succeeded() const { return snapshot_.has_value(); }
	const std::optional<LSystemBranchGraphGenerationSnapshot> &snapshot() const
	{
		return snapshot_;
	}
	const std::string &diagnostic() const { return diagnostic_; }

private:
	LSystemBranchGraphGenerationResult(
		std::optional<LSystemBranchGraphGenerationSnapshot> snapshot,
		std::string diagnostic)
		: snapshot_(std::move(snapshot)), diagnostic_(std::move(diagnostic))
	{
	}

	std::optional<LSystemBranchGraphGenerationSnapshot> snapshot_;
	std::string diagnostic_;
};
