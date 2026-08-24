#pragma once

#include "vegetation/model/BranchGraph.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

class RuleBranchingGenerationResult
{
public:
	static RuleBranchingGenerationResult succeeded(
		BranchGraph graph,
		std::uint64_t topology_hash)
	{
		return RuleBranchingGenerationResult(
			std::move(graph), {}, topology_hash);
	}

	static RuleBranchingGenerationResult failed(std::string diagnostic)
	{
		return RuleBranchingGenerationResult(
			std::nullopt, std::move(diagnostic), 0u);
	}

	bool succeeded() const { return graph_.has_value(); }
	const std::optional<BranchGraph> &graph() const { return graph_; }
	const std::string &diagnostic() const { return diagnostic_; }
	std::uint64_t topologyHash() const { return topology_hash_; }

private:
	RuleBranchingGenerationResult(
		std::optional<BranchGraph> graph,
		std::string diagnostic,
		std::uint64_t topology_hash)
		: graph_(std::move(graph)),
		  diagnostic_(std::move(diagnostic)),
		  topology_hash_(topology_hash)
	{
	}

	std::optional<BranchGraph> graph_;
	std::string diagnostic_;
	std::uint64_t topology_hash_ = 0;
};

