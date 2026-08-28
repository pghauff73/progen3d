#pragma once

#include "vehicle/mcsmv2/model/ModernCarSemanticValidationReport.h"
#include "vehicle/mcsmv2/model/VehiclePanelPatchGraph.h"

class VehiclePanelPatchValidationService
{
public:
	ModernCarSemanticValidationReport validate(
		const VehiclePanelPatchGraph &graph) const;
};
