#pragma once

#include <cstddef>
#include <cstdint>

class SpaceColonizationResolutionEvidence
{
public:
	SpaceColonizationResolutionEvidence(
		std::uint64_t source_graph_hash,
		std::uint64_t resolved_graph_hash,
		std::uint64_t evidence_hash,
		std::size_t attraction_point_count,
		std::size_t remaining_attraction_point_count,
		std::size_t completed_iterations,
		std::size_t added_node_count,
		std::size_t obstacle_rejected_candidate_count)
		: source_graph_hash_(source_graph_hash),
		  resolved_graph_hash_(resolved_graph_hash),
		  evidence_hash_(evidence_hash),
		  attraction_point_count_(attraction_point_count),
		  remaining_attraction_point_count_(remaining_attraction_point_count),
		  completed_iterations_(completed_iterations),
		  added_node_count_(added_node_count),
		  obstacle_rejected_candidate_count_(obstacle_rejected_candidate_count)
	{
	}

	std::uint64_t sourceGraphHash() const { return source_graph_hash_; }
	std::uint64_t resolvedGraphHash() const { return resolved_graph_hash_; }
	std::uint64_t evidenceHash() const { return evidence_hash_; }
	std::size_t attractionPointCount() const { return attraction_point_count_; }
	std::size_t remainingAttractionPointCount() const
	{
		return remaining_attraction_point_count_;
	}
	std::size_t completedIterations() const { return completed_iterations_; }
	std::size_t addedNodeCount() const { return added_node_count_; }
	std::size_t obstacleRejectedCandidateCount() const
	{
		return obstacle_rejected_candidate_count_;
	}

private:
	std::uint64_t source_graph_hash_ = 0;
	std::uint64_t resolved_graph_hash_ = 0;
	std::uint64_t evidence_hash_ = 0;
	std::size_t attraction_point_count_ = 0;
	std::size_t remaining_attraction_point_count_ = 0;
	std::size_t completed_iterations_ = 0;
	std::size_t added_node_count_ = 0;
	std::size_t obstacle_rejected_candidate_count_ = 0;
};
