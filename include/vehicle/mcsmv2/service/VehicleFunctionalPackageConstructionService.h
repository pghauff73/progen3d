#pragma once

#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"
#include "vehicle/mcsmv2/model/VehicleFunctionalPackageSystem.h"

class VehicleFunctionalPackageConstructionService
{
public:
	VehicleFunctionalPackageSystem construct(
		const ModernCarSemanticVariant &variant) const;
};
