#pragma once

#include "geometry/model/ShapeSpecification.h"
#include "vegetation/model/PlantSpeciesSpecification.h"
#include "vegetation/model/VegetationObstacleBoundary.h"
#include "vegetation/model/VegetationSurfaceTarget.h"
#include "vegetation/model/VineGrowthSpecification.h"

#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <utility>
#include <vector>

class VineShapeSpecification : public ShapeSpecification
{
public:
	VineShapeSpecification(
		PlantSpeciesSpecification species,
		glm::vec3 start_position,
		glm::vec3 initial_direction,
		float initial_radius,
		VineGrowthSpecification growth,
		std::optional<VegetationSurfaceTarget> target,
		std::vector<VegetationObstacleBoundary> obstacles,
		ShapeSpecificationKey key,
		std::string canonical_text,
		GeometryDetailLevel detail_level)
		: ShapeSpecification(ShapeFamily::Vine, std::move(key), detail_level),
		  species_(std::move(species)),
		  start_position_(start_position),
		  initial_direction_(initial_direction),
		  initial_radius_(initial_radius),
		  growth_(std::move(growth)),
		  target_(std::move(target)),
		  obstacles_(std::move(obstacles)),
		  canonical_text_(std::move(canonical_text))
	{
	}

	const PlantSpeciesSpecification &species() const { return species_; }
	const glm::vec3 &startPosition() const { return start_position_; }
	const glm::vec3 &initialDirection() const { return initial_direction_; }
	float initialRadius() const { return initial_radius_; }
	const VineGrowthSpecification &growth() const { return growth_; }
	const std::optional<VegetationSurfaceTarget> &target() const { return target_; }
	const std::vector<VegetationObstacleBoundary> &obstacles() const
	{
		return obstacles_;
	}

	std::string canonicalText() const override { return canonical_text_; }
	bool isDefaultFamilyShape() const override { return false; }
	bool requestsClosedGeometry() const override { return false; }

private:
	PlantSpeciesSpecification species_;
	glm::vec3 start_position_{0.0f};
	glm::vec3 initial_direction_{0.0f, 1.0f, 0.0f};
	float initial_radius_ = 0.0f;
	VineGrowthSpecification growth_{
		VineGrowthMode::FreeClimbing,
		VegetationCollisionBehavior::Avoid,
		SurfaceAttachmentMode::Offset,
		0.1f, 1u, 0.0f, 0.0f, 0.001f, 0.9f, 0.001f,
		glm::vec3(0.0f, 1.0f, 0.0f)};
	std::optional<VegetationSurfaceTarget> target_;
	std::vector<VegetationObstacleBoundary> obstacles_;
	std::string canonical_text_;
};
