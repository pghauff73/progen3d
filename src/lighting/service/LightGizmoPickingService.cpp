#include "lighting/service/LightGizmoPickingService.h"

#include <limits>

LightId LightGizmoPickingService::pick(
	const LightingSceneState &lighting_state,
	const LightGizmoProjectionContext &projection_context,
	float selection_radius_pixels) const
{
	if (!lighting_state.light_gizmos_visible ||
	    projection_context.viewport_size.x <= 1.0f ||
	    projection_context.viewport_size.y <= 1.0f) {
		return LightId();
	}
	LightId nearest_id;
	float nearest_depth = std::numeric_limits<float>::max();
	for (const SceneLight &light : lighting_state.lights.lights()) {
		if (!light.visible()) continue;
		const glm::vec3 position = GpuLightRecordFactory().resolvePosition(
			light, projection_context.light_camera);
		const glm::vec4 clip = projection_context.view_projection * glm::vec4(position, 1.0f);
		if (clip.w <= 0.0001f) continue;
		const glm::vec3 ndc = glm::vec3(clip) / clip.w;
		if (ndc.z < -1.0f || ndc.z > 1.0f) continue;
		const glm::vec2 screen(
			(ndc.x * 0.5f + 0.5f) * projection_context.viewport_size.x,
			(1.0f - (ndc.y * 0.5f + 0.5f)) * projection_context.viewport_size.y);
		const float pointer_distance = glm::length(screen - projection_context.pointer_position);
		if (pointer_distance <= selection_radius_pixels && ndc.z < nearest_depth) {
			nearest_depth = ndc.z;
			nearest_id = light.id();
		}
	}
	return nearest_id;
}
