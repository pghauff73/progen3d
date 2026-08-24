#pragma once

#include "vehicle/model/ModernVehicleAssembly.h"
#include "vehicle/model/WheelAssemblySpecification.h"

class VehicleValidationService
{
public:
	VehicleValidationReport validate(
		const VehicleDefinition &definition,
		const VehicleBodySpecification &body_specification,
		const WheelAssemblySpecification &wheel_specification,
		const std::vector<VehiclePlacedAssembly> &assemblies,
		const std::vector<VehicleJoint> &joints,
		const GeneratedPrimitiveMesh &combined_mesh) const;
};
