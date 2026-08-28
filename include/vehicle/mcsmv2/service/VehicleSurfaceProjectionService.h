#pragma once

#include "Mesh.h"
#include "vehicle/mcsmv2/model/VehicleSurfaceProjection.h"

#include <glm/glm.hpp>

#include <optional>
#include <vector>

class VehicleSurfaceProjectionService
{
public:
	std::optional<VehicleSurfaceProjection> projectPoint(
		const Mesh &target_surface,
		const glm::dvec3 &query_point) const;

	std::vector<VehicleSurfaceProjection> projectPoints(
		const Mesh &target_surface,
		const std::vector<glm::dvec3> &query_points) const;
};
