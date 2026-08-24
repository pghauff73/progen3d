#pragma once

#include "vehicle/parametric/model/GeneratedVehicleRealization.h"
#include "vehicle/parametric/model/ModernCarVariantDefinition.h"

class ModernCarBodySectionExtractionService
{
public:
	GeneratedBodySectionSet extract(
		const ModernCarVariantDefinition &variant,
		const GeneratedBodyMesh &body_mesh) const;
};
