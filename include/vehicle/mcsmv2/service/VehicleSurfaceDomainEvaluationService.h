#pragma once

#include "vehicle/mcsmv2/model/VehicleSurfaceDomain.h"

#include <glm/glm.hpp>

#include <vector>

class VehicleSurfaceDomainEvaluationService
{
public:
	bool contains(
		const VehicleSurfaceDomain &domain,
		const glm::dvec2 &surface_coordinate) const;

	glm::dvec2 interpolateFaceCoordinate(
		const glm::ivec3 &face,
		const glm::dvec3 &barycentric_coordinates,
		const std::vector<glm::dvec2> &vertex_surface_coordinates) const;

	glm::dvec2 calculateFaceCentroidCoordinate(
		const glm::ivec3 &face,
		const std::vector<glm::dvec2> &vertex_surface_coordinates) const;
};
