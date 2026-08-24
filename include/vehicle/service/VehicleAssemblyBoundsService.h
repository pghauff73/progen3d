#pragma once

#include "geometry/model/AxisAlignedBounds.h"
#include "vehicle/model/ModernVehicleAssembly.h"

class VehicleAssemblyBoundsService
{
public:
	AxisAlignedBounds calculate(
		const VehiclePlacedAssembly &assembly) const;
};
