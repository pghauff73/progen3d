#pragma once

#include "spatial/model/CollisionLayer.h"
#include "spatial/model/CollisionLayerMask.h"

class ScatterCollisionPolicy
{
public:
	ScatterCollisionPolicy(
		CollisionLayer placement_layer,
		CollisionLayerMask collision_mask)
		: placement_layer_(placement_layer),
		  collision_mask_(collision_mask)
	{
	}

	CollisionLayer placementLayer() const { return placement_layer_; }
	const CollisionLayerMask &collisionMask() const { return collision_mask_; }

private:
	CollisionLayer placement_layer_ = CollisionLayer::Terrain;
	CollisionLayerMask collision_mask_;
};
