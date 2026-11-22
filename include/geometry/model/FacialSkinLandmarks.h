#pragma once

#include <glm/glm.hpp>

// Local coordinates generated from the same model as the skin. Eyeball centers
// stay fixed in the skull frame while the aperture margins follow expressions.
struct FacialSkinLandmarks
{
	glm::vec3 left_eyeball_center{0.0f};
	glm::vec3 right_eyeball_center{0.0f};
	float eyeball_radius = 0.0f;
	glm::vec3 left_eye_outer_corner{0.0f};
	glm::vec3 left_eye_inner_corner{0.0f};
	glm::vec3 left_eye_upper_margin{0.0f};
	glm::vec3 left_eye_lower_margin{0.0f};
	glm::vec3 right_eye_outer_corner{0.0f};
	glm::vec3 right_eye_inner_corner{0.0f};
	glm::vec3 right_eye_upper_margin{0.0f};
	glm::vec3 right_eye_lower_margin{0.0f};
	glm::vec3 mouth_left_corner{0.0f};
	glm::vec3 mouth_right_corner{0.0f};
	glm::vec3 mouth_upper_margin{0.0f};
	glm::vec3 mouth_lower_margin{0.0f};
};
