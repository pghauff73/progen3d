#pragma once

#include "vehicle/parametric/model/GeneratedVehicleRealization.h"
#include "vehicle/parametric/model/ModernCarVariantDefinition.h"

class ModernCarObservationGenerationService
{
public:
	GeneratedObservationSet generate(
		const ModernCarVariantDefinition &variant,
		const GeneratedBodyMesh &body_mesh) const;
};
