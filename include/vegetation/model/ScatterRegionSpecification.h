#pragma once

#include "vegetation/model/ScatterCollisionPolicy.h"
#include "vegetation/model/ScatterOrientationMode.h"
#include "vegetation/model/ScatterSurfaceFace.h"
#include "vegetation/model/VegetationSurfaceTarget.h"

#include <glm/glm.hpp>

#include <string>
#include <utility>

class ScatterRegionSpecification
{
public:
	ScatterRegionSpecification(
		std::string identifier,
		std::string object_identifier,
		VegetationSurfaceTarget surface,
		ScatterSurfaceFace surface_face,
		float density,
		float minimum_distance,
		glm::vec2 scale_range,
		ScatterOrientationMode orientation_mode,
		float collision_radius,
		ScatterCollisionPolicy collision_policy)
		: identifier_(std::move(identifier)),
		  object_identifier_(std::move(object_identifier)),
		  surface_(std::move(surface)),
		  surface_face_(surface_face),
		  density_(density),
		  minimum_distance_(minimum_distance),
		  scale_range_(scale_range),
		  orientation_mode_(orientation_mode),
		  collision_radius_(collision_radius),
		  collision_policy_(std::move(collision_policy))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &objectIdentifier() const { return object_identifier_; }
	const VegetationSurfaceTarget &surface() const { return surface_; }
	ScatterSurfaceFace surfaceFace() const { return surface_face_; }
	float density() const { return density_; }
	float minimumDistance() const { return minimum_distance_; }
	const glm::vec2 &scaleRange() const { return scale_range_; }
	ScatterOrientationMode orientationMode() const { return orientation_mode_; }
	float collisionRadius() const { return collision_radius_; }
	const ScatterCollisionPolicy &collisionPolicy() const
	{
		return collision_policy_;
	}

private:
	std::string identifier_;
	std::string object_identifier_;
	VegetationSurfaceTarget surface_;
	ScatterSurfaceFace surface_face_ = ScatterSurfaceFace::MaximumY;
	float density_ = 0.0f;
	float minimum_distance_ = 0.0f;
	glm::vec2 scale_range_{1.0f};
	ScatterOrientationMode orientation_mode_ =
		ScatterOrientationMode::SurfaceNormal;
	float collision_radius_ = 0.0f;
	ScatterCollisionPolicy collision_policy_{
		CollisionLayer::Terrain, CollisionLayerMask()};
};
