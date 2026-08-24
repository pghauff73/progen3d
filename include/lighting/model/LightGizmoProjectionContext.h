#pragma once

#include "lighting/service/GpuLightRecordFactory.h"

#include <glm/glm.hpp>

class LightGizmoProjectionContext
{
public:
	PreviewLightCameraContext light_camera;
	glm::mat4 view_projection{1.0f};
	glm::vec2 viewport_size{1.0f};
	glm::vec2 pointer_position{0.0f};
};
