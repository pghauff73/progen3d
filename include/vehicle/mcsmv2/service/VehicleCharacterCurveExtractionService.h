#pragma once

#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"
#include "vehicle/mcsmv2/model/VehicleCharacterCurveNetwork.h"

#include <cstddef>

class VehicleCharacterCurveExtractionService
{
public:
	VehicleCharacterCurveNetwork extract(
		const ModernCarSemanticVariant &variant,
		std::size_t samples_per_curve = 96u) const;
};
