#pragma once

#include <glm/glm.hpp>

class SpatialInterfaceWorldFrame {
public:
	SpatialInterfaceWorldFrame(glm::vec3 origin,
	                           glm::vec3 normal,
	                           glm::vec3 tangent,
	                           glm::vec3 bitangent)
		: origin_(origin),
		  normal_(normal),
		  tangent_(tangent),
		  bitangent_(bitangent) {}

	const glm::vec3 &origin() const { return origin_; }
	const glm::vec3 &normal() const { return normal_; }
	const glm::vec3 &tangent() const { return tangent_; }
	const glm::vec3 &bitangent() const { return bitangent_; }

private:
	glm::vec3 origin_{0.0f};
	glm::vec3 normal_{0.0f, 1.0f, 0.0f};
	glm::vec3 tangent_{1.0f, 0.0f, 0.0f};
	glm::vec3 bitangent_{0.0f, 0.0f, 1.0f};
};
