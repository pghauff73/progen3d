#pragma once

#include "lighting/model/PreviewLightCollection.h"
#include "lighting/model/PreviewLightingFrame.h"
#include "lighting/service/GpuLightRecordFactory.h"

class PreviewLightingFrameFactory
{
public:
	PreviewLightingFrame build(const PreviewLightCollection &lights,
	                           const PreviewLightCameraContext &camera) const;

private:
	glm::mat4 buildPrimaryShadowViewProjection(
		const SceneLight &light,
		const glm::vec3 &resolved_position,
		const PreviewLightCameraContext &camera) const;
};
