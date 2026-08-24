#pragma once

#include "lighting/model/GpuLightRecord.h"
#include "lighting/model/SceneLight.h"

#include <glm/glm.hpp>

#include <cstdint>

class PreviewLightCameraContext
{
public:
	glm::vec3 camera_position{0.0f};
	glm::vec3 camera_target{0.0f};
	glm::vec3 camera_right{1.0f, 0.0f, 0.0f};
	glm::vec3 camera_up{0.0f, 1.0f, 0.0f};
	glm::vec3 camera_view_direction{0.0f, 0.0f, 1.0f};
	float orbit_distance = 5.0f;
	float scene_scale = 1.0f;
};

class ShadowAssignment
{
public:
	std::int32_t shadow_index = -1;
};

class GpuLightRecordFactory
{
public:
	GpuLightRecord make(const SceneLight &light,
	                    const PreviewLightCameraContext &camera,
	                    const ShadowAssignment &shadow_assignment) const;

	glm::vec3 resolvePosition(const SceneLight &light,
	                         const PreviewLightCameraContext &camera) const;
	float convertIntensity(const SceneLight &light,
	                      const PreviewLightCameraContext &camera,
	                      const glm::vec3 &resolved_position) const;
};
