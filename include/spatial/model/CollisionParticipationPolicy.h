#pragma once

#include "spatial/model/CollisionLayer.h"
#include "spatial/model/CollisionLayerMask.h"

class CollisionParticipationPolicy {
public:
	CollisionParticipationPolicy(CollisionLayer layer, CollisionLayerMask mask)
		: layer_(layer), mask_(mask) {}

	CollisionLayer layer() const { return layer_; }
	const CollisionLayerMask &mask() const { return mask_; }

	bool blocks(const CollisionParticipationPolicy &other) const
	{
		return mask_.contains(other.layer_) && other.mask_.contains(layer_);
	}

private:
	CollisionLayer layer_ = CollisionLayer::Temporary;
	CollisionLayerMask mask_;
};
