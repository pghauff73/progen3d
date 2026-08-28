#pragma once

#include <glm/glm.hpp>

class alignas(16) GpuLightRecord
{
public:
	glm::vec4 position_and_type{0.0f};
	glm::vec4 direction_and_range{0.0f};
	glm::vec4 color_and_intensity{0.0f};
	glm::vec4 cone_and_area{0.0f};
	glm::uvec4 metadata{0u};
};

static_assert(sizeof(GpuLightRecord) == 80, "GpuLightRecord must match the GLSL std430 layout.");
