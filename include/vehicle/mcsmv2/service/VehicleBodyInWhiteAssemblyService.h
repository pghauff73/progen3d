#pragma once

#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"
#include "vehicle/mcsmv2/model/VehicleBodyInWhiteAssembly.h"

class VehicleBodyInWhiteAssemblyService
{
public:
	VehicleBodyInWhiteAssembly assemble(
		const ModernCarSemanticVariant &variant) const;
};
