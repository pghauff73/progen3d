#pragma once

#include <glm/glm.hpp>

#include <algorithm>

// The camera axes are orthonormal and expressed in world coordinates.
struct CalibratedOrthographicView {
	int width;
	int height;
	float pixels_per_unit;
	glm::vec3 center;
	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 forward;
	float distance;

	static constexpr float near_clip = 0.01f;
	float farClip() const { return std::max(100.0f, distance * 4.5f + 24.0f); }
	float depth(const glm::vec3 &point) const { return distance + glm::dot(point - center, forward); }

	glm::vec2 imagePoint(const glm::vec3 &point) const
	{
		const auto offset = point - center;
		return {width * 0.5f + glm::dot(offset, right) * pixels_per_unit,
		        height * 0.5f - glm::dot(offset, up) * pixels_per_unit};
	}
};
