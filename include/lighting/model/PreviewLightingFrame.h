#pragma once

#include "lighting/model/GpuLightRecord.h"
#include "lighting/model/LightId.h"
#include "lighting/model/ShadowAssignmentPlan.h"

#include <glm/glm.hpp>

#include <vector>

class PreviewLightingFrame
{
public:
	std::vector<GpuLightRecord> gpu_light_records;
	ShadowAssignmentPlan shadow_assignment_plan;
	LightId primary_shadow_light_id;
	glm::mat4 primary_shadow_view_projection{1.0f};
	bool primary_shadow_available = false;
};
