#pragma once

#include "vehicle/model/VehicleMvp26Architecture.h"
#include "vehicle/model/VehicleValidationReport.h"

class VehicleMvp26ValidationService
{
public:
	VehicleValidationReport validate(
		const VehicleMvp26Architecture &architecture) const;
};
