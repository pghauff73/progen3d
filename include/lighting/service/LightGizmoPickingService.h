#pragma once

#include "lighting/model/LightGizmoProjectionContext.h"
#include "lighting/model/LightingSceneState.h"

class LightGizmoPickingService
{
public:
	LightId pick(const LightingSceneState &lighting_state,
	             const LightGizmoProjectionContext &projection_context,
	             float selection_radius_pixels = 10.0f) const;
};
