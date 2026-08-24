#pragma once

#include "geometry/model/GeneratedPrimitiveMesh.h"
#include "vehicle/model/ModernVehicleAssembly.h"
#include "vehicle/model/VehicleDefinition.h"
#include "vehicle/model/VehicleJoint.h"

#include <cstdint>
#include <vector>

class VehicleDeterministicHashService
{
public:
	std::uint64_t calculate(
		const VehicleDefinition &definition,
		const std::vector<VehiclePlacedAssembly> &assemblies,
		const std::vector<VehicleJoint> &joints,
		const GeneratedPrimitiveMesh &combined_mesh) const;
};
