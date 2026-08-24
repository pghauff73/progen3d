#pragma once

#include "vegetation/model/InflorescenceKind.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <string>
#include <utility>

class InflorescencePlacement
{
public:
	InflorescencePlacement(
		std::string identifier,
		InflorescenceKind kind,
		std::size_t sequence_index,
		glm::vec3 position,
		glm::vec3 direction,
		float scale,
		glm::mat4 local_transform)
		: identifier_(std::move(identifier)),
		  kind_(kind),
		  sequence_index_(sequence_index),
		  position_(position),
		  direction_(direction),
		  scale_(scale),
		  local_transform_(local_transform)
	{
	}

	const std::string &identifier() const { return identifier_; }
	InflorescenceKind kind() const { return kind_; }
	std::size_t sequenceIndex() const { return sequence_index_; }
	const glm::vec3 &position() const { return position_; }
	const glm::vec3 &direction() const { return direction_; }
	float scale() const { return scale_; }
	const glm::mat4 &localTransform() const { return local_transform_; }

private:
	std::string identifier_;
	InflorescenceKind kind_ = InflorescenceKind::Raceme;
	std::size_t sequence_index_ = 0u;
	glm::vec3 position_{0.0f};
	glm::vec3 direction_{0.0f, 1.0f, 0.0f};
	float scale_ = 1.0f;
	glm::mat4 local_transform_{1.0f};
};
