#pragma once

#include "geometry/model/ShapeSpecification.h"
#include "vegetation/model/PlantSpeciesSpecification.h"
#include "vegetation/model/ScatterObstacleBoundary.h"
#include "vegetation/model/ScatterRegionSpecification.h"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class ScatterRegionShapeSpecification : public ShapeSpecification
{
public:
	ScatterRegionShapeSpecification(
		PlantSpeciesSpecification species,
		ScatterRegionSpecification scatter,
		std::uint64_t deterministic_seed,
		std::vector<ScatterObstacleBoundary> obstacles,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level)
		: ShapeSpecification(
			ShapeFamily::ScatterRegion, std::move(key), detail_level),
		  species_(std::move(species)),
		  scatter_(std::move(scatter)),
		  deterministic_seed_(deterministic_seed),
		  obstacles_(std::move(obstacles)),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const PlantSpeciesSpecification &species() const { return species_; }
	const ScatterRegionSpecification &scatter() const { return scatter_; }
	std::uint64_t deterministicSeed() const { return deterministic_seed_; }
	const std::vector<ScatterObstacleBoundary> &obstacles() const
	{
		return obstacles_;
	}

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override { return false; }

private:
	PlantSpeciesSpecification species_;
	ScatterRegionSpecification scatter_;
	std::uint64_t deterministic_seed_ = 0u;
	std::vector<ScatterObstacleBoundary> obstacles_;
	std::string canonical_text_;
};
