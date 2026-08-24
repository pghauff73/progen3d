#pragma once

#include "lighting/model/LightGizmoSegment.h"
#include "lighting/model/LightingSceneState.h"
#include "lighting/service/GpuLightRecordFactory.h"

#include <vector>

class LightGizmoOverlayService
{
public:
	std::vector<LightGizmoSegment> build(
		const LightingSceneState &lighting_state,
		const PreviewLightCameraContext &camera) const;
};
