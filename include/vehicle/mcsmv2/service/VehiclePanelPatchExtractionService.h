#pragma once

#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"
#include "vehicle/mcsmv2/model/VehiclePanelPatchGraph.h"

class VehiclePanelPatchExtractionService
{
public:
	VehiclePanelPatchGraph extract(
		const ModernCarSemanticVariant &variant) const;
};
