#pragma once

#include "vegetation/model/VegetationOrganArrayHost.h"
#include "vegetation/model/VegetationOrganType.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <string>
#include <utility>

class OrganArrayPlacement
{
public:
	OrganArrayPlacement(
		std::string identifier,
		VegetationOrganType organ_type,
		VegetationOrganArrayHost host,
		std::size_t sequence_index,
		float host_distance,
		float azimuth_degrees,
		float scale,
		glm::vec3 position,
		glm::vec3 direction,
		glm::mat4 local_transform)
		: identifier_(std::move(identifier)),
		  organ_type_(organ_type),
		  host_(host),
		  sequence_index_(sequence_index),
		  host_distance_(host_distance),
		  azimuth_degrees_(azimuth_degrees),
		  scale_(scale),
		  position_(position),
		  direction_(direction),
		  local_transform_(local_transform)
	{
	}

	const std::string &identifier() const { return identifier_; }
	VegetationOrganType organType() const { return organ_type_; }
	VegetationOrganArrayHost host() const { return host_; }
	std::size_t sequenceIndex() const { return sequence_index_; }
	float hostDistance() const { return host_distance_; }
	float azimuthDegrees() const { return azimuth_degrees_; }
	float scale() const { return scale_; }
	const glm::vec3 &position() const { return position_; }
	const glm::vec3 &direction() const { return direction_; }
	const glm::mat4 &localTransform() const { return local_transform_; }

private:
	std::string identifier_;
	VegetationOrganType organ_type_ = VegetationOrganType::Leaf;
	VegetationOrganArrayHost host_ = VegetationOrganArrayHost::Branch;
	std::size_t sequence_index_ = 0u;
	float host_distance_ = 0.0f;
	float azimuth_degrees_ = 0.0f;
	float scale_ = 1.0f;
	glm::vec3 position_{0.0f};
	glm::vec3 direction_{0.0f, 1.0f, 0.0f};
	glm::mat4 local_transform_{1.0f};
};
