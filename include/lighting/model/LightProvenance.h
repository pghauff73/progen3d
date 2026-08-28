#pragma once

#include <cstdint>

enum class LightProvenance : std::uint32_t
{
	Grammar = 0,
	Editor = 1,
	Preset = 2,
	Imported = 3
};
