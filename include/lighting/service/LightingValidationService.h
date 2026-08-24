#pragma once

#include "lighting/model/LightingValidationReport.h"
#include "lighting/model/PreviewLightCollection.h"

class LightingValidationService
{
public:
	LightingValidationReport validate(const PreviewLightCollection &lights) const;
};
