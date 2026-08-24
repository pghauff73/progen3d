#pragma once

#include "geometry/model/GeometryComplexityLimits.h"
#include "vegetation/model/PlantShapeSpecification.h"
#include "vegetation/model/VegetationComplexityLimits.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct PlantShapeSpecificationCandidate
{
	std::optional<PlantSpeciesSpecification> species;
	float age = 0.0f;
	PlantDevelopmentState development_state = PlantDevelopmentState::Mature;
	std::uint64_t deterministic_seed = 1u;
	PlantTopologyGenerationSpecification topology_generation =
		PlantTopologyGenerationSpecification::createRuleBranching();
	std::optional<PlantGrowthSpecification> growth;
	std::vector<TropismInfluence> tropism_influences;
	GeometryDetailLevel detail_level = GeometryDetailLevel::Component;
};

class PlantShapeSpecificationValidator
{
public:
	explicit PlantShapeSpecificationValidator(
		GeometryComplexityLimits geometry_complexity_limits =
			GeometryComplexityLimits(),
		VegetationComplexityLimits vegetation_complexity_limits =
			VegetationComplexityLimits())
		: geometry_complexity_limits_(geometry_complexity_limits),
		  vegetation_complexity_limits_(vegetation_complexity_limits)
	{
	}

	std::shared_ptr<const PlantShapeSpecification> validate(
		PlantShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits geometry_complexity_limits_;
	VegetationComplexityLimits vegetation_complexity_limits_;
};
