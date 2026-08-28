#pragma once

#include <glm/glm.hpp>

#include <optional>

class TriangleGeometryRelationshipService
{
public:
	double squaredDistance(
		const glm::dvec3 &first_a,
		const glm::dvec3 &first_b,
		const glm::dvec3 &first_c,
		const glm::dvec3 &second_a,
		const glm::dvec3 &second_b,
		const glm::dvec3 &second_c) const;

	bool intersects(
		const glm::dvec3 &first_a,
		const glm::dvec3 &first_b,
		const glm::dvec3 &first_c,
		const glm::dvec3 &second_a,
		const glm::dvec3 &second_b,
		const glm::dvec3 &second_c,
		double tolerance = 1.0e-10) const;

	std::optional<double> rayIntersectionDistance(
		const glm::dvec3 &origin,
		const glm::dvec3 &direction,
		const glm::dvec3 &triangle_a,
		const glm::dvec3 &triangle_b,
		const glm::dvec3 &triangle_c,
		double tolerance = 1.0e-10) const;

	double pointSquaredDistance(
		const glm::dvec3 &point,
		const glm::dvec3 &triangle_a,
		const glm::dvec3 &triangle_b,
		const glm::dvec3 &triangle_c) const;
};
