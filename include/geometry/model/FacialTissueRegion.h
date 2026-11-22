#pragma once

#include <glm/glm.hpp>

#include <string>

struct FacialTissueRegion
{
	std::string name;
	glm::vec2 center_uv{0.5f};
	glm::vec2 influence_radius_uv{0.1f};
	float young_modulus_scale = 1.0f;
	float support_stiffness_scale = 1.0f;
};
