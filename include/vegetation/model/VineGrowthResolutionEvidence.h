#pragma once

#include "vegetation/model/VinePathState.h"

#include <cstddef>
#include <cstdint>

class VineGrowthResolutionEvidence
{
public:
	VineGrowthResolutionEvidence(
		std::uint64_t source_graph_hash,
		std::uint64_t resolved_graph_hash,
		std::uint64_t evidence_hash,
		std::size_t added_segment_count,
		std::size_t attachment_count,
		std::size_t avoided_collision_count,
		VinePathState path_state)
		: source_graph_hash_(source_graph_hash),
		  resolved_graph_hash_(resolved_graph_hash),
		  evidence_hash_(evidence_hash),
		  added_segment_count_(added_segment_count),
		  attachment_count_(attachment_count),
		  avoided_collision_count_(avoided_collision_count),
		  path_state_(path_state)
	{
	}

	std::uint64_t sourceGraphHash() const { return source_graph_hash_; }
	std::uint64_t resolvedGraphHash() const { return resolved_graph_hash_; }
	std::uint64_t evidenceHash() const { return evidence_hash_; }
	std::size_t addedSegmentCount() const { return added_segment_count_; }
	std::size_t attachmentCount() const { return attachment_count_; }
	std::size_t avoidedCollisionCount() const
	{
		return avoided_collision_count_;
	}
	VinePathState pathState() const { return path_state_; }

private:
	std::uint64_t source_graph_hash_ = 0;
	std::uint64_t resolved_graph_hash_ = 0;
	std::uint64_t evidence_hash_ = 0;
	std::size_t added_segment_count_ = 0;
	std::size_t attachment_count_ = 0;
	std::size_t avoided_collision_count_ = 0;
	VinePathState path_state_ = VinePathState::Free;
};
