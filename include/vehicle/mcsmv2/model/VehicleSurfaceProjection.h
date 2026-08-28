#pragma once

#include <glm/glm.hpp>

#include <cstddef>

class VehicleSurfaceProjection
{
public:
	VehicleSurfaceProjection(
		glm::dvec3 projected_point,
		std::size_t face_index,
		glm::dvec3 barycentric_coordinates,
		double distance)
		: projected_point_(projected_point),
		  face_index_(face_index),
		  barycentric_coordinates_(barycentric_coordinates),
		  distance_(distance)
	{
	}

	const glm::dvec3 &projectedPoint() const { return projected_point_; }
	std::size_t faceIndex() const { return face_index_; }
	const glm::dvec3 &barycentricCoordinates() const
	{
		return barycentric_coordinates_;
	}
	double distance() const { return distance_; }

private:
	glm::dvec3 projected_point_{0.0};
	std::size_t face_index_ = 0u;
	glm::dvec3 barycentric_coordinates_{1.0, 0.0, 0.0};
	double distance_ = 0.0;
};
