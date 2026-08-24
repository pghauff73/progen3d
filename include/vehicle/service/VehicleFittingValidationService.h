#pragma once

#include "vehicle/model/VehicleFittingArchitecture.h"
#include "vehicle/model/VehicleValidationReport.h"

class VehicleFittingValidationService
{
public:
	VehicleValidationReport validate(
		const VehicleFittingArchitecture &architecture) const;
};
