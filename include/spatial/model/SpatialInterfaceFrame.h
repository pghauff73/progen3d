#pragma once

#include <glm/glm.hpp>

class SpatialInterfaceFrame {
public:
	SpatialInterfaceFrame(glm::vec3 local_origin,
	                      glm::vec3 local_normal,
	                      glm::vec3 local_tangent)
		: local_origin_(local_origin),
		  local_normal_(local_normal),
		  local_tangent_(local_tangent) {}

	const glm::vec3 &localOrigin() const { return local_origin_; }
	const glm::vec3 &localNormal() const { return local_normal_; }
	const glm::vec3 &localTangent() const { return local_tangent_; }
	glm::vec3 localBitangent() const { return glm::cross(local_normal_, local_tangent_); }

private:
	glm::vec3 local_origin_{0.0f};
	glm::vec3 local_normal_{0.0f, 1.0f, 0.0f};
	glm::vec3 local_tangent_{1.0f, 0.0f, 0.0f};
};
