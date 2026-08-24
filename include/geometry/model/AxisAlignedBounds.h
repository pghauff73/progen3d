#pragma once

#include <glm/glm.hpp>

struct AxisAlignedBounds {
	glm::vec3 min{0.0f};
	glm::vec3 max{0.0f};
	glm::vec3 center{0.0f};
	glm::vec3 half_extents{0.0f};
	bool valid = false;
};

using PrimitiveBounds = AxisAlignedBounds;
