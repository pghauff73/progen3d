#pragma once

#include "vehicle/model/VehicleDatum.h"
#include "vehicle/model/VehiclePackage.h"
#include "vehicle/model/VehicleStyleState.h"

class VehicleDatumDerivationService
{
public:
	VehicleDatumSet derive(
		const VehiclePackage &package,
		const VehicleStyleState &style) const;
};
