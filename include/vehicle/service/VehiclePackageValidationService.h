#pragma once

#include "vehicle/model/VehicleDefinition.h"
#include "vehicle/model/VehicleValidationReport.h"

class VehiclePackageValidationService
{
public:
	VehicleValidationReport validate(
		const VehicleIntent &intent,
		const VehiclePackage &package) const;
};
