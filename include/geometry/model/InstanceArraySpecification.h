#pragma once

#include <glm/glm.hpp>

#include <utility>
#include <vector>

class InstanceArraySpecification
{
public:
	explicit InstanceArraySpecification(std::vector<glm::mat4> transforms)
		: transforms_(std::move(transforms))
	{
	}

	static InstanceArraySpecification createLinear(
		std::size_t count,
		glm::vec3 spacing);
	static InstanceArraySpecification createGrid(
		std::size_t x_count,
		std::size_t y_count,
		glm::vec2 spacing);
	static InstanceArraySpecification createRadial(
		std::size_t count,
		float radius,
		glm::vec3 axis,
		float start_degrees = 0.0f);

	const std::vector<glm::mat4> &transforms() const { return transforms_; }

private:
	std::vector<glm::mat4> transforms_;
};
