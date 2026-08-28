#include "geometry/model/InstanceArraySpecification.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

InstanceArraySpecification InstanceArraySpecification::createLinear(
	std::size_t count,
	glm::vec3 spacing)
{
	std::vector<glm::mat4> transforms;
	transforms.reserve(count);
	for (std::size_t index = 0; index < count; ++index) {
		transforms.push_back(glm::translate(
			glm::mat4(1.0f), spacing * static_cast<float>(index)));
	}
	return InstanceArraySpecification(std::move(transforms));
}

InstanceArraySpecification InstanceArraySpecification::createGrid(
	std::size_t x_count,
	std::size_t y_count,
	glm::vec2 spacing)
{
	std::vector<glm::mat4> transforms;
	transforms.reserve(x_count * y_count);
	for (std::size_t y_index = 0; y_index < y_count; ++y_index) {
		for (std::size_t x_index = 0; x_index < x_count; ++x_index) {
			transforms.push_back(glm::translate(
				glm::mat4(1.0f),
				glm::vec3(
					spacing.x * static_cast<float>(x_index),
					spacing.y * static_cast<float>(y_index),
					0.0f)));
		}
	}
	return InstanceArraySpecification(std::move(transforms));
}

InstanceArraySpecification InstanceArraySpecification::createRadial(
	std::size_t count,
	float radius,
	glm::vec3 axis,
	float start_degrees)
{
	std::vector<glm::mat4> transforms;
	if (count == 0u || !std::isfinite(radius) || radius < 0.0f ||
	    glm::length(axis) <= 1.0e-6f || !std::isfinite(start_degrees)) {
		return InstanceArraySpecification(std::move(transforms));
	}
	axis = glm::normalize(axis);
	transforms.reserve(count);
	for (std::size_t index = 0; index < count; ++index) {
		const float angle = glm::radians(
			start_degrees + 360.0f * static_cast<float>(index) /
				static_cast<float>(count));
		glm::vec3 radial_direction;
		if (std::fabs(axis.y) > 0.9f) {
			radial_direction = glm::vec3(std::sin(angle), 0.0f, std::cos(angle));
		}
		else if (std::fabs(axis.x) > 0.9f) {
			radial_direction = glm::vec3(0.0f, std::sin(angle), std::cos(angle));
		}
		else {
			radial_direction = glm::vec3(std::cos(angle), std::sin(angle), 0.0f);
		}
		transforms.push_back(
			glm::translate(glm::mat4(1.0f), radial_direction * radius) *
			glm::rotate(glm::mat4(1.0f), angle, axis));
	}
	return InstanceArraySpecification(std::move(transforms));
}
