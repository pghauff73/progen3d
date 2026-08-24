#pragma once

#include "geometry/model/AxisAlignedBounds.h"
#include "spatial/model/CollisionLayer.h"

#include <string>
#include <utility>

class ScatterObstacleBoundary
{
public:
	ScatterObstacleBoundary(
		std::string identifier,
		AxisAlignedBounds bounds,
		CollisionLayer collision_layer)
		: identifier_(std::move(identifier)),
		  bounds_(bounds),
		  collision_layer_(collision_layer)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const AxisAlignedBounds &bounds() const { return bounds_; }
	CollisionLayer collisionLayer() const { return collision_layer_; }

private:
	std::string identifier_;
	AxisAlignedBounds bounds_;
	CollisionLayer collision_layer_ = CollisionLayer::Terrain;
};
