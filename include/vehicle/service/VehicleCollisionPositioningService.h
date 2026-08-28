#pragma once

#include "vehicle/model/VehicleAssemblyPlacementResult.h"

#include <glm/glm.hpp>

#include <string>

class VehicleCollisionPositioningService
{
public:
	VehicleAssemblyPlacementResult positionAgainst(
		const std::string &constraint_identifier,
		const VehiclePlacedAssembly &moving_assembly,
		const VehiclePlacedAssembly &target_assembly,
		glm::vec3 direction,
		float clearance,
		float maximum_distance,
		float tolerance = 0.0005f) const;
};
