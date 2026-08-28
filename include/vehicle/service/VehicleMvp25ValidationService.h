#pragma once

#include "vehicle/model/VehicleMvp25Architecture.h"
#include "vehicle/model/VehicleValidationReport.h"

class VehicleMvp25ValidationService
{
public:
	VehicleValidationReport validate(
		const VehicleMvp25Architecture &architecture) const;
};
