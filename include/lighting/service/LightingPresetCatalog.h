#pragma once

#include "lighting/model/LightingPresetDefinition.h"
#include "lighting/model/PreviewLightCollection.h"

#include <string>
#include <vector>

class LightingPresetCatalog
{
public:
	const std::vector<LightingPresetDefinition> &presets() const;
	bool apply(const std::string &preset_name, PreviewLightCollection *lights) const;
};
