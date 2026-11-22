#pragma once

#include <glm/glm.hpp>

#include <string>

// A tensile cable from a neutral skull origin to a compact skin insertion
// patch. Coordinates and influence radii use the unchanged rest texture UVs.
struct FacialMuscleSpecification
{
	std::string name;
	glm::vec2 origin_uv{0.0f};
	glm::vec2 insertion_uv{0.0f};
	glm::vec2 influence_radius_uv{0.1f};
	float activation = 0.0f;
	float maximum_shortening = 0.0f;
	float stiffness_n_per_m = 0.0f;
};
