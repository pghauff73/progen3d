#pragma once

#include <cstddef>
#include <cstdint>

class LSystemBranchGraphGenerationEvidence
{
public:
	LSystemBranchGraphGenerationEvidence(
		std::size_t iteration_count,
		std::size_t expanded_symbol_count,
		std::size_t maximum_stack_depth,
		std::size_t generated_segment_count,
		std::uint64_t topology_hash)
		: iteration_count_(iteration_count),
		  expanded_symbol_count_(expanded_symbol_count),
		  maximum_stack_depth_(maximum_stack_depth),
		  generated_segment_count_(generated_segment_count),
		  topology_hash_(topology_hash)
	{
	}

	std::size_t iterationCount() const { return iteration_count_; }
	std::size_t expandedSymbolCount() const { return expanded_symbol_count_; }
	std::size_t maximumStackDepth() const { return maximum_stack_depth_; }
	std::size_t generatedSegmentCount() const
	{
		return generated_segment_count_;
	}
	std::uint64_t topologyHash() const { return topology_hash_; }

private:
	std::size_t iteration_count_ = 0u;
	std::size_t expanded_symbol_count_ = 0u;
	std::size_t maximum_stack_depth_ = 0u;
	std::size_t generated_segment_count_ = 0u;
	std::uint64_t topology_hash_ = 0u;
};
