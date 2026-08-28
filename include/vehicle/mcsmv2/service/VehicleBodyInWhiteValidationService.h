#pragma once

#include "vehicle/mcsmv2/model/ModernCarSemanticValidationReport.h"
#include "vehicle/mcsmv2/model/VehicleBodyInWhiteAssembly.h"

class VehicleBodyInWhiteValidationService
{
public:
	ModernCarSemanticValidationReport validate(
		const VehicleBodyInWhiteAssembly &assembly) const;
};
