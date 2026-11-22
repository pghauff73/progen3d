#pragma once

#include <glm/glm.hpp>

#include <string>

// A compact material-space attachment to the neutral skull reference.
struct FacialAttachmentPatch
{
	std::string name;
	glm::vec2 center_uv{0.5f};
	glm::vec2 influence_radius_uv{0.1f};
	float stiffness_n_per_m = 0.0f;
};
