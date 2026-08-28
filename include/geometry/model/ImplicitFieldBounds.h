#pragma once

#include <glm/glm.hpp>

#include <cmath>

class ImplicitFieldBounds
{
public:
	ImplicitFieldBounds(
		glm::dvec3 minimum = glm::dvec3(0.0),
		glm::dvec3 maximum = glm::dvec3(0.0))
		: minimum_(minimum),
		  maximum_(maximum)
	{
	}

	const glm::dvec3 &minimum() const { return minimum_; }
	const glm::dvec3 &maximum() const { return maximum_; }
	glm::dvec3 dimensions() const { return maximum_ - minimum_; }

	bool isFinite() const
	{
		return std::isfinite(minimum_.x) && std::isfinite(minimum_.y) &&
		       std::isfinite(minimum_.z) && std::isfinite(maximum_.x) &&
		       std::isfinite(maximum_.y) && std::isfinite(maximum_.z);
	}

	bool hasPositiveExtent() const
	{
		const glm::dvec3 extent = dimensions();
		return extent.x > 0.0 && extent.y > 0.0 && extent.z > 0.0;
	}

	bool isValid() const { return isFinite() && hasPositiveExtent(); }

private:
	glm::dvec3 minimum_{0.0};
	glm::dvec3 maximum_{0.0};
};
