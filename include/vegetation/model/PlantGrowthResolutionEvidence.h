#pragma once

#include "vegetation/model/PlantDevelopmentState.h"

#include <cstdint>

class PlantGrowthResolutionEvidence
{
public:
	PlantGrowthResolutionEvidence(
		std::uint64_t source_graph_hash,
		std::uint64_t resolved_graph_hash,
		std::uint64_t evidence_hash,
		float evaluation_time,
		float normalized_progress,
		float length_factor,
		float radius_factor,
		float organ_scale_factor,
		PlantDevelopmentState development_state)
		: source_graph_hash_(source_graph_hash),
		  resolved_graph_hash_(resolved_graph_hash),
		  evidence_hash_(evidence_hash),
		  evaluation_time_(evaluation_time),
		  normalized_progress_(normalized_progress),
		  length_factor_(length_factor),
		  radius_factor_(radius_factor),
		  organ_scale_factor_(organ_scale_factor),
		  development_state_(development_state)
	{
	}

	std::uint64_t sourceGraphHash() const { return source_graph_hash_; }
	std::uint64_t resolvedGraphHash() const { return resolved_graph_hash_; }
	std::uint64_t evidenceHash() const { return evidence_hash_; }
	float evaluationTime() const { return evaluation_time_; }
	float normalizedProgress() const { return normalized_progress_; }
	float lengthFactor() const { return length_factor_; }
	float radiusFactor() const { return radius_factor_; }
	float organScaleFactor() const { return organ_scale_factor_; }
	PlantDevelopmentState developmentState() const
	{
		return development_state_;
	}

private:
	std::uint64_t source_graph_hash_ = 0;
	std::uint64_t resolved_graph_hash_ = 0;
	std::uint64_t evidence_hash_ = 0;
	float evaluation_time_ = 0.0f;
	float normalized_progress_ = 0.0f;
	float length_factor_ = 0.0f;
	float radius_factor_ = 0.0f;
	float organ_scale_factor_ = 0.0f;
	PlantDevelopmentState development_state_ = PlantDevelopmentState::Seed;
};
