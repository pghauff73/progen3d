#pragma once

#include "geometry/model/ShapeSpecification.h"
#include "vegetation/model/PlantDevelopmentState.h"
#include "vegetation/model/PlantGrowthSpecification.h"
#include "vegetation/model/PlantSpeciesSpecification.h"
#include "vegetation/model/PlantTopologyGenerationSpecification.h"
#include "vegetation/model/TropismInfluence.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class PlantShapeSpecification : public ShapeSpecification
{
public:
	PlantShapeSpecification(
		PlantSpeciesSpecification species,
		float age,
		PlantDevelopmentState development_state,
		std::uint64_t deterministic_seed,
		PlantTopologyGenerationSpecification topology_generation,
		std::optional<PlantGrowthSpecification> growth,
		std::vector<TropismInfluence> tropism_influences,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level)
		: ShapeSpecification(ShapeFamily::Plant, std::move(key), detail_level),
		  species_(std::move(species)),
		  age_(age),
		  development_state_(development_state),
		  deterministic_seed_(deterministic_seed),
		  topology_generation_(std::move(topology_generation)),
		  growth_(std::move(growth)),
		  tropism_influences_(std::move(tropism_influences)),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const PlantSpeciesSpecification &species() const { return species_; }
	float age() const { return age_; }
	PlantDevelopmentState developmentState() const { return development_state_; }
	std::uint64_t deterministicSeed() const { return deterministic_seed_; }
	const PlantTopologyGenerationSpecification &topologyGeneration() const
	{
		return topology_generation_;
	}
	const std::optional<PlantGrowthSpecification> &growth() const
	{
		return growth_;
	}
	const std::vector<TropismInfluence> &tropismInfluences() const
	{
		return tropism_influences_;
	}

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override { return false; }

private:
	PlantSpeciesSpecification species_;
	float age_ = 0.0f;
	PlantDevelopmentState development_state_ = PlantDevelopmentState::Seed;
	std::uint64_t deterministic_seed_ = 0u;
	PlantTopologyGenerationSpecification topology_generation_ =
		PlantTopologyGenerationSpecification::createRuleBranching();
	std::optional<PlantGrowthSpecification> growth_;
	std::vector<TropismInfluence> tropism_influences_;
	std::string canonical_text_;
};
