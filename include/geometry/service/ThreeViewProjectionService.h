#pragma once

#include "Mesh.h"
#include "geometry/model/ThreeViewProjection.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <optional>

class OrthographicProjectionEnvelope
{
public:
	OrthographicProjectionEnvelope(glm::vec3 minimum, glm::vec3 maximum)
		: minimum_(minimum), maximum_(maximum)
	{
	}
	const glm::vec3 &minimum() const { return minimum_; }
	const glm::vec3 &maximum() const { return maximum_; }
private:
	glm::vec3 minimum_{-0.5f};
	glm::vec3 maximum_{0.5f};
};

class ThreeViewProjectionConfiguration
{
public:
	ThreeViewProjectionConfiguration(std::size_t resolution = 256u,
	                                 float padding_ratio = 0.04f,
	                                 std::optional<OrthographicProjectionEnvelope> envelope = std::nullopt)
		: resolution_(resolution), padding_ratio_(padding_ratio),
		  envelope_(std::move(envelope))
	{
	}

	std::size_t resolution() const { return resolution_; }
	float paddingRatio() const { return padding_ratio_; }
	const std::optional<OrthographicProjectionEnvelope> &envelope() const { return envelope_; }

private:
	std::size_t resolution_ = 256u;
	float padding_ratio_ = 0.04f;
	std::optional<OrthographicProjectionEnvelope> envelope_;
};

class ThreeViewProjectionService
{
public:
	ThreeViewProjection project(
		const Mesh &mesh,
		const glm::mat4 &world_transform,
		const ThreeViewProjectionConfiguration &configuration = {}) const;
};
