#pragma once

#include "vegetation/model/PlantSpeciesSpecification.h"
#include "vegetation/model/VegetationComplexityLimits.h"
#include "vegetation/model/VegetationObstacleBoundary.h"
#include "vegetation/model/VegetationSurfaceTarget.h"
#include "vegetation/model/VineGrowthMode.h"
#include "vegetation/model/VineShapeSpecification.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct VineShapeSpecificationCandidate
{
	std::optional<PlantSpeciesSpecification> species;
	glm::vec3 start_position{0.0f};
	glm::vec3 initial_direction{0.0f, 1.0f, 0.0f};
	float initial_radius = 0.032f;
	VineGrowthMode growth_mode = VineGrowthMode::FreeClimbing;
	VegetationCollisionBehavior collision_behavior =
		VegetationCollisionBehavior::Avoid;
	SurfaceAttachmentMode attachment_mode = SurfaceAttachmentMode::Offset;
	float step_length = 0.14f;
	std::size_t maximum_segments = 18u;
	float maximum_seek_distance = 1.0f;
	float attachment_distance = 0.02f;
	float attachment_tolerance = 0.001f;
	float radius_decay = 0.92f;
	float minimum_radius = 0.004f;
	glm::vec3 preferred_direction{0.0f, 1.0f, 0.0f};
	float radius_conservation_exponent = 2.0f;
	std::optional<VegetationSurfaceTarget> target;
	std::vector<VegetationObstacleBoundary> obstacles;
	GeometryDetailLevel detail_level = GeometryDetailLevel::Component;
};

class VineShapeSpecificationValidator
{
public:
	explicit VineShapeSpecificationValidator(
		VegetationComplexityLimits complexity_limits =
			VegetationComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const VineShapeSpecification> validate(
		VineShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	VegetationComplexityLimits complexity_limits_;
};
