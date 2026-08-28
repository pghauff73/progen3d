#pragma once

#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"
#include "vehicle/mcsmv2/model/VehicleWheelEnvelopeSet.h"

class VehicleWheelMotionEnvelopeConstructionService
{
public:
	VehicleWheelEnvelopeSet construct(
		const ModernCarSemanticVariant &variant) const;
};
