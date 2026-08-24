#pragma once

#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/GeometryDetailLevel.h"
#include "vegetation/model/PlantDevelopmentState.h"
#include "vegetation/model/PlantSpeciesSpecification.h"
#include "vegetation/model/VegetationAssemblyGeometry.h"

#include <cstdint>

class BranchGraph;

class VegetationGeometryAssemblyService
{
public:
	explicit VegetationGeometryAssemblyService(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	VegetationGeometryBuildResult build(
		const BranchGraph &graph,
		const PlantSpeciesSpecification &species,
		GeometryDetailLevel detail_level) const;

	VegetationGeometryBuildResult build(
		const BranchGraph &graph,
		const PlantSpeciesSpecification &species,
		GeometryDetailLevel detail_level,
		float plant_age,
		PlantDevelopmentState development_state,
		std::uint64_t deterministic_seed = 0u) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
