#pragma once

#include "vehicle/mcsmv2/model/ModernCarSemanticValidationReport.h"
#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"
#include "vehicle/mcsmv2/model/VehicleFunctionalPackageSystem.h"

class VehicleFunctionalPackageValidationService
{
public:
	ModernCarSemanticValidationReport validate(
		const ModernCarSemanticVariant &variant,
		const VehicleFunctionalPackageSystem &system) const;
};
