#pragma once

enum class VegetationOrganOrientation
{
	Outward,
	AlongHost,
	WorldUp,
	WorldDown,
	SurfaceNormal
};

inline const char *vegetationOrganOrientationName(
	VegetationOrganOrientation orientation)
{
	switch (orientation) {
	case VegetationOrganOrientation::Outward: return "Outward";
	case VegetationOrganOrientation::AlongHost: return "AlongHost";
	case VegetationOrganOrientation::WorldUp: return "WorldUp";
	case VegetationOrganOrientation::WorldDown: return "WorldDown";
	case VegetationOrganOrientation::SurfaceNormal: return "SurfaceNormal";
	}
	return "Unknown";
}
