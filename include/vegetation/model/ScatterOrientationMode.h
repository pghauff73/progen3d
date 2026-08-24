#pragma once

enum class ScatterOrientationMode
{
	SurfaceNormal,
	SurfaceNormalRandomAzimuth,
	WorldUpRandomAzimuth
};

inline const char *scatterOrientationModeName(ScatterOrientationMode mode)
{
	switch (mode) {
	case ScatterOrientationMode::SurfaceNormal: return "SurfaceNormal";
	case ScatterOrientationMode::SurfaceNormalRandomAzimuth:
		return "SurfaceNormalRandomAzimuth";
	case ScatterOrientationMode::WorldUpRandomAzimuth:
		return "WorldUpRandomAzimuth";
	}
	return "Unknown";
}
