#pragma once

#include "vegetation/model/PlantDevelopmentState.h"
#include "vegetation/model/PlantSpeciesSpecification.h"
#include "vegetation/model/TropismInfluence.h"

#include <cstdint>
#include <utility>
#include <vector>

class RuleBranchingGenerationRequest
{
public:
	RuleBranchingGenerationRequest(
		PlantSpeciesSpecification species,
		float age,
		PlantDevelopmentState development_state,
		std::uint64_t deterministic_seed,
		std::vector<TropismInfluence> tropism_influences = {})
		: species_(std::move(species)),
		  age_(age),
		  development_state_(development_state),
		  deterministic_seed_(deterministic_seed),
		  tropism_influences_(std::move(tropism_influences))
	{
	}

	const PlantSpeciesSpecification &species() const { return species_; }
	float age() const { return age_; }
	PlantDevelopmentState developmentState() const { return development_state_; }
	std::uint64_t deterministicSeed() const { return deterministic_seed_; }
	const std::vector<TropismInfluence> &tropismInfluences() const
	{
		return tropism_influences_;
	}

private:
	PlantSpeciesSpecification species_;
	float age_ = 0.0f;
	PlantDevelopmentState development_state_ = PlantDevelopmentState::Seed;
	std::uint64_t deterministic_seed_ = 0;
	std::vector<TropismInfluence> tropism_influences_;
};
