#pragma once

#include "spatial/model/CollisionLayer.h"

#include <cstdint>
#include <initializer_list>

class CollisionLayerMask {
public:
	CollisionLayerMask() = default;
	explicit CollisionLayerMask(std::initializer_list<CollisionLayer> layers)
	{
		for (CollisionLayer layer : layers) add(layer);
	}

	void add(CollisionLayer layer)
	{
		bits_ |= bitFor(layer);
	}

	bool contains(CollisionLayer layer) const
	{
		return (bits_ & bitFor(layer)) != 0;
	}

	std::uint32_t bits() const { return bits_; }

	static CollisionLayerMask all()
	{
		CollisionLayerMask mask;
		mask.bits_ = (1u << 10u) - 1u;
		return mask;
	}

private:
	static std::uint32_t bitFor(CollisionLayer layer)
	{
		return 1u << static_cast<std::uint8_t>(layer);
	}

	std::uint32_t bits_ = 0;
};
