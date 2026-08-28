#pragma once

enum class ScatterSurfaceFace
{
	MinimumX,
	MaximumX,
	MinimumY,
	MaximumY,
	MinimumZ,
	MaximumZ
};

inline const char *scatterSurfaceFaceName(ScatterSurfaceFace face)
{
	switch (face) {
	case ScatterSurfaceFace::MinimumX: return "MinimumX";
	case ScatterSurfaceFace::MaximumX: return "MaximumX";
	case ScatterSurfaceFace::MinimumY: return "MinimumY";
	case ScatterSurfaceFace::MaximumY: return "MaximumY";
	case ScatterSurfaceFace::MinimumZ: return "MinimumZ";
	case ScatterSurfaceFace::MaximumZ: return "MaximumZ";
	}
	return "Unknown";
}
