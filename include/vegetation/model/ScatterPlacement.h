#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>

class ScatterPlacement
{
public:
	ScatterPlacement(
		std::string identifier,
		std::string object_identifier,
		std::string surface_identifier,
		glm::vec3 position,
		glm::vec3 surface_normal,
		float scale,
		float azimuth_degrees,
		glm::mat4 local_transform)
		: identifier_(std::move(identifier)),
		  object_identifier_(std::move(object_identifier)),
		  surface_identifier_(std::move(surface_identifier)),
		  position_(position),
		  surface_normal_(surface_normal),
		  scale_(scale),
		  azimuth_degrees_(azimuth_degrees),
		  local_transform_(local_transform)
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::string &objectIdentifier() const { return object_identifier_; }
	const std::string &surfaceIdentifier() const { return surface_identifier_; }
	const glm::vec3 &position() const { return position_; }
	const glm::vec3 &surfaceNormal() const { return surface_normal_; }
	float scale() const { return scale_; }
	float azimuthDegrees() const { return azimuth_degrees_; }
	const glm::mat4 &localTransform() const { return local_transform_; }

private:
	std::string identifier_;
	std::string object_identifier_;
	std::string surface_identifier_;
	glm::vec3 position_{0.0f};
	glm::vec3 surface_normal_{0.0f, 1.0f, 0.0f};
	float scale_ = 1.0f;
	float azimuth_degrees_ = 0.0f;
	glm::mat4 local_transform_{1.0f};
};
