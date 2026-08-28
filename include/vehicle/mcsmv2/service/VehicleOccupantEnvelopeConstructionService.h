#pragma once

#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"
#include "vehicle/mcsmv2/model/VehicleOccupantEnvelopeSystem.h"

class VehicleOccupantEnvelopeConstructionService
{
public:
	VehicleOccupantEnvelopeSystem construct(
		const ModernCarSemanticVariant &variant) const;
};
