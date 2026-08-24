#pragma once

#include "vehicle/parametric/model/GeneratedVehicleRealization.h"
#include "vehicle/parametric/model/ModernCarVariantDefinition.h"

class ModernCarCharacterCurveExtractionService
{
public:
	GeneratedCharacterCurveSet extract(
		const ModernCarVariantDefinition &variant) const;
};
