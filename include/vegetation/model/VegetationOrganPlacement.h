#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <string>
#include <utility>

class VegetationOrganPlacement
{
public:
	VegetationOrganPlacement(
		std::string identifier,
		std::size_t node_index,
		std::size_t organ_index_at_node,
		float path_distance,
		float azimuth_degrees,
		glm::vec3 position,
		glm::vec3 direction,
		glm::mat4 local_transform)
		: identifier_(std::move(identifier)),
		  node_index_(node_index),
		  organ_index_at_node_(organ_index_at_node),
		  path_distance_(path_distance),
		  azimuth_degrees_(azimuth_degrees),
		  position_(position),
		  direction_(direction),
		  local_transform_(local_transform)
	{
	}

	const std::string &identifier() const { return identifier_; }
	std::size_t nodeIndex() const { return node_index_; }
	std::size_t organIndexAtNode() const { return organ_index_at_node_; }
	float pathDistance() const { return path_distance_; }
	float azimuthDegrees() const { return azimuth_degrees_; }
	const glm::vec3 &position() const { return position_; }
	const glm::vec3 &direction() const { return direction_; }
	const glm::mat4 &localTransform() const { return local_transform_; }

private:
	std::string identifier_;
	std::size_t node_index_ = 0u;
	std::size_t organ_index_at_node_ = 0u;
	float path_distance_ = 0.0f;
	float azimuth_degrees_ = 0.0f;
	glm::vec3 position_{0.0f};
	glm::vec3 direction_{0.0f, 1.0f, 0.0f};
	glm::mat4 local_transform_{1.0f};
};

