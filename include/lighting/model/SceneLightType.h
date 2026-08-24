#pragma once

#include <cstdint>

enum class SceneLightType : std::uint32_t
{
	Directional = 0,
	Point = 1,
	Spot = 2,
	RectangularArea = 3
};
