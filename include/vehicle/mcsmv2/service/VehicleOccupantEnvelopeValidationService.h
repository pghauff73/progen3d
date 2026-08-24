#pragma once

#include "vehicle/mcsmv2/model/ModernCarSemanticValidationReport.h"
#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"
#include "vehicle/mcsmv2/model/VehicleOccupantEnvelopeSystem.h"

class VehicleOccupantEnvelopeValidationService
{
public:
	ModernCarSemanticValidationReport validate(
		const ModernCarSemanticVariant &variant,
		const VehicleOccupantEnvelopeSystem &system) const;
};
