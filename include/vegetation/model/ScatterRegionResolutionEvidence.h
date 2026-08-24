#pragma once

#include <cstddef>
#include <cstdint>

class ScatterRegionResolutionEvidence
{
public:
	ScatterRegionResolutionEvidence(
		std::uint64_t deterministic_seed,
		std::uint64_t evidence_hash,
		std::size_t requested_placement_count,
		std::size_t accepted_placement_count,
		std::size_t evaluated_candidate_count,
		std::size_t spacing_rejection_count,
		std::size_t collision_rejection_count)
		: deterministic_seed_(deterministic_seed),
		  evidence_hash_(evidence_hash),
		  requested_placement_count_(requested_placement_count),
		  accepted_placement_count_(accepted_placement_count),
		  evaluated_candidate_count_(evaluated_candidate_count),
		  spacing_rejection_count_(spacing_rejection_count),
		  collision_rejection_count_(collision_rejection_count)
	{
	}

	std::uint64_t deterministicSeed() const { return deterministic_seed_; }
	std::uint64_t evidenceHash() const { return evidence_hash_; }
	std::size_t requestedPlacementCount() const
	{
		return requested_placement_count_;
	}
	std::size_t acceptedPlacementCount() const
	{
		return accepted_placement_count_;
	}
	std::size_t evaluatedCandidateCount() const
	{
		return evaluated_candidate_count_;
	}
	std::size_t spacingRejectionCount() const
	{
		return spacing_rejection_count_;
	}
	std::size_t collisionRejectionCount() const
	{
		return collision_rejection_count_;
	}

private:
	std::uint64_t deterministic_seed_ = 0;
	std::uint64_t evidence_hash_ = 0;
	std::size_t requested_placement_count_ = 0;
	std::size_t accepted_placement_count_ = 0;
	std::size_t evaluated_candidate_count_ = 0;
	std::size_t spacing_rejection_count_ = 0;
	std::size_t collision_rejection_count_ = 0;
};
