#pragma once

#include <glm/glm.hpp>

enum class SpatialDirectionFrame {
	World,
	Parent,
	MovingObject,
	MovingInterface,
	TargetObject,
	TargetInterface
};

class SpatialDirection {
public:
	SpatialDirection(SpatialDirectionFrame frame, glm::vec3 vector)
		: frame_(frame), vector_(vector) {}

	SpatialDirectionFrame frame() const { return frame_; }
	const glm::vec3 &vector() const { return vector_; }

private:
	SpatialDirectionFrame frame_ = SpatialDirectionFrame::World;
	glm::vec3 vector_{0.0f};
};
