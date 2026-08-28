#pragma once

enum class VegetationCollisionBehavior
{
	Avoid,
	Seek,
	PermitIntersection
};

inline const char *vegetationCollisionBehaviorName(
	VegetationCollisionBehavior behavior)
{
	switch (behavior) {
	case VegetationCollisionBehavior::Avoid: return "Avoid";
	case VegetationCollisionBehavior::Seek: return "Seek";
	case VegetationCollisionBehavior::PermitIntersection:
		return "PermitIntersection";
	}
	return "Unknown";
}
