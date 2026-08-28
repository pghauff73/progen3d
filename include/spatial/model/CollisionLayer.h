#pragma once

#include <cstdint>

enum class CollisionLayer : std::uint8_t {
	Structure,
	Envelope,
	Interior,
	Furniture,
	Plumbing,
	Hvac,
	Electrical,
	Equipment,
	Terrain,
	Temporary
};

inline const char *collisionLayerName(CollisionLayer layer)
{
	switch (layer) {
	case CollisionLayer::Structure: return "Structure";
	case CollisionLayer::Envelope: return "Envelope";
	case CollisionLayer::Interior: return "Interior";
	case CollisionLayer::Furniture: return "Furniture";
	case CollisionLayer::Plumbing: return "Plumbing";
	case CollisionLayer::Hvac: return "Hvac";
	case CollisionLayer::Electrical: return "Electrical";
	case CollisionLayer::Equipment: return "Equipment";
	case CollisionLayer::Terrain: return "Terrain";
	case CollisionLayer::Temporary: return "Temporary";
	}
	return "Unknown";
}
