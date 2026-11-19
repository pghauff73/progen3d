#pragma once

#include "lighting/model/LightingSceneState.h"

class SceneGenerationContext;

class ElectricalControlsPanel
{
public:
	void draw(LightingSceneState *lighting_state, const SceneGenerationContext *context = nullptr) const;
};
