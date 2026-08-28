#pragma once

#include "vegetation/model/PlantSpeciesSpecification.h"
#include "vegetation/model/ScatterObstacleBoundary.h"
#include "vegetation/model/ScatterOrientationMode.h"
#include "vegetation/model/ScatterRegionShapeSpecification.h"
#include "vegetation/model/ScatterSurfaceFace.h"
#include "vegetation/model/VegetationComplexityLimits.h"
#include "vegetation/model/VegetationSurfaceTarget.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct ScatterRegionShapeSpecificationCandidate
{
	std::optional<PlantSpeciesSpecification> species;
	std::string region_identifier = "vegetation_scatter";
	std::optional<VegetationSurfaceTarget> surface;
	ScatterSurfaceFace surface_face = ScatterSurfaceFace::MaximumY;
	float density = 1.0f;
	float minimum_distance = 0.25f;
	glm::vec2 scale_range{1.0f};
	ScatterOrientationMode orientation_mode =
		ScatterOrientationMode::WorldUpRandomAzimuth;
	float collision_radius = 0.0f;
	CollisionLayer placement_layer = CollisionLayer::Terrain;
	CollisionLayerMask collision_mask = CollisionLayerMask::all();
	std::uint64_t deterministic_seed = 1u;
	std::vector<ScatterObstacleBoundary> obstacles;
	GeometryDetailLevel detail_level = GeometryDetailLevel::Assembly;
};

class ScatterRegionShapeSpecificationValidator
{
public:
	explicit ScatterRegionShapeSpecificationValidator(
		VegetationComplexityLimits complexity_limits =
			VegetationComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const ScatterRegionShapeSpecification> validate(
		ScatterRegionShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	VegetationComplexityLimits complexity_limits_;
};
