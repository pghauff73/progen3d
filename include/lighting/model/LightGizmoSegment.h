#pragma once

#include <glm/glm.hpp>

class LightGizmoSegment
{
public:
	glm::vec3 start{0.0f};
	glm::vec3 end{0.0f};
	glm::vec4 color{1.0f};
};
