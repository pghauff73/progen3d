#pragma once

#include <array>

#include <glm/glm.hpp>

struct CollisionOrientedBox {
	glm::vec3 center{0.0f};
	std::array<glm::vec3, 3> axes{
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f)};
	glm::vec3 half_extents{0.0f};
	bool valid = false;
};

using CollisionObbData = CollisionOrientedBox;
