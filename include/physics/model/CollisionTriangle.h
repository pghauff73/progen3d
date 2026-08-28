#pragma once

#include "geometry/model/AxisAlignedBounds.h"

#include <array>

#include <glm/glm.hpp>

struct CollisionTriangle {
	std::array<glm::vec3, 3> vertices{};
	AxisAlignedBounds bounds;
	glm::vec3 centroid{0.0f};
	glm::vec3 normal{0.0f};
	bool valid = false;
};
