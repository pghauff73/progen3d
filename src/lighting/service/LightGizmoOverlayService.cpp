#include "lighting/service/LightGizmoOverlayService.h"

#include "lighting/service/PhotometricColorService.h"

#include <glm/gtc/quaternion.hpp>

#include <algorithm>

namespace {

void append_segment(std::vector<LightGizmoSegment> *segments,
	                const glm::vec3 &start,
	                const glm::vec3 &end,
	                const glm::vec4 &color)
{
	segments->push_back(LightGizmoSegment{start, end, color});
}

glm::vec3 display_color(const SceneLight &light)
{
	const glm::vec3 color = light.emission().usesColorTemperature()
		? PhotometricColorService().kelvinToLinearRgb(light.emission().colorTemperatureKelvin())
		: light.emission().linearRgb();
	return glm::clamp(color, glm::vec3(0.15f), glm::vec3(1.0f));
}

}

std::vector<LightGizmoSegment> LightGizmoOverlayService::build(
	const LightingSceneState &lighting_state,
	const PreviewLightCameraContext &camera) const
{
	std::vector<LightGizmoSegment> segments;
	if (!lighting_state.light_gizmos_visible) return segments;
	const float scene_scale = std::max(camera.scene_scale, 0.001f);
	const float marker_size = std::max(camera.orbit_distance * 0.018f / scene_scale, 0.08f);
	for (const SceneLight &light : lighting_state.lights.lights()) {
		if (!light.visible()) continue;
		const glm::vec3 position = GpuLightRecordFactory().resolvePosition(light, camera) / scene_scale;
		const bool selected = light.id() == lighting_state.selected_light_id;
		const bool hovered = light.id() == lighting_state.hovered_light_id;
		const glm::vec3 base_color = display_color(light);
		const glm::vec4 color(
			selected ? glm::vec3(1.0f, 0.68f, 0.16f) :
			hovered ? glm::vec3(1.0f, 0.92f, 0.35f) : base_color,
			1.0f);
		const float size = marker_size * (selected ? 1.35f : 1.0f);
		append_segment(&segments, position - glm::vec3(size, 0.0f, 0.0f), position + glm::vec3(size, 0.0f, 0.0f), color);
		append_segment(&segments, position - glm::vec3(0.0f, size, 0.0f), position + glm::vec3(0.0f, size, 0.0f), color);
		append_segment(&segments, position - glm::vec3(0.0f, 0.0f, size), position + glm::vec3(0.0f, 0.0f, size), color);

		if (light.type() == SceneLightType::Directional || light.type() == SceneLightType::Spot ||
		    light.type() == SceneLightType::RectangularArea) {
			const glm::vec3 direction = light.emissionDirection();
			append_segment(&segments, position, position + direction * size * 3.0f, color);
		}
		if (const auto *area = std::get_if<RectangularAreaLightData>(&light.data())) {
			const glm::mat3 orientation = glm::mat3_cast(light.orientation());
			const glm::vec3 right = orientation * glm::vec3(1.0f, 0.0f, 0.0f) * area->dimensions.x * 0.5f;
			const glm::vec3 forward = orientation * glm::vec3(0.0f, 0.0f, 1.0f) * area->dimensions.y * 0.5f;
			const glm::vec3 corners[] = {
				position - right - forward,
				position + right - forward,
				position + right + forward,
				position - right + forward};
			for (int index = 0; index < 4; ++index) {
				append_segment(&segments, corners[index], corners[(index + 1) % 4], color);
			}
		}
	}
	return segments;
}
