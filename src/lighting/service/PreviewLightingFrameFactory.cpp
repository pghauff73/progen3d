#include "lighting/service/PreviewLightingFrameFactory.h"

#include "lighting/service/ShadowAssignmentService.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace {

glm::vec3 safe_direction(const glm::vec3 &direction)
{
	return glm::length(direction) > 0.0001f
		? glm::normalize(direction)
		: glm::vec3(0.0f, -1.0f, 0.0f);
}

glm::vec3 shadow_up_direction(const glm::vec3 &direction)
{
	return std::fabs(glm::dot(direction, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.98f
		? glm::vec3(0.0f, 0.0f, 1.0f)
		: glm::vec3(0.0f, 1.0f, 0.0f);
}

}

PreviewLightingFrame PreviewLightingFrameFactory::build(
	const PreviewLightCollection &lights,
	const PreviewLightCameraContext &camera) const
{
	PreviewLightingFrame frame;
	frame.shadow_assignment_plan = ShadowAssignmentService().assign(lights);
	const SceneLight *primary_shadow_light = nullptr;
	for (const SceneLight &light : lights.lights()) {
		if (!light.enabled()) continue;
		if (frame.gpu_light_records.size() >= PreviewLightCollection::maximum_visible_lights) break;
		const ShadowAssignmentRecord *shadow_record =
			frame.shadow_assignment_plan.find(light.id());
		if (shadow_record != nullptr && shadow_record->rendered_by_current_backend) {
			primary_shadow_light = &light;
			frame.primary_shadow_light_id = light.id();
			frame.primary_shadow_available = true;
		}
		const ShadowAssignment assignment{
			shadow_record != nullptr && shadow_record->rendered_by_current_backend ? 0 : -1};
		frame.gpu_light_records.push_back(
			GpuLightRecordFactory().make(light, camera, assignment));
	}
	if (primary_shadow_light != nullptr) {
		const glm::vec3 resolved_position =
			GpuLightRecordFactory().resolvePosition(*primary_shadow_light, camera);
		frame.primary_shadow_view_projection = buildPrimaryShadowViewProjection(
			*primary_shadow_light, resolved_position, camera);
	}
	return frame;
}

glm::mat4 PreviewLightingFrameFactory::buildPrimaryShadowViewProjection(
	const SceneLight &light,
	const glm::vec3 &resolved_position,
	const PreviewLightCameraContext &camera) const
{
	const float scene_scale = std::max(camera.scene_scale, 0.001f);
	const float shadow_extent = std::max(
		10.0f, std::max(12.0f * scene_scale, camera.orbit_distance * 3.2f));
	const float far_plane = std::max(
		40.0f, std::max(48.0f * scene_scale, camera.orbit_distance * 6.5f));

	if (light.type() == SceneLightType::Directional) {
		const glm::vec3 direction = safe_direction(light.emissionDirection());
		const glm::vec3 position = camera.camera_target - direction * shadow_extent;
		const glm::mat4 view = glm::lookAt(
			position, camera.camera_target, shadow_up_direction(direction));
		return glm::ortho(
			-shadow_extent, shadow_extent, -shadow_extent, shadow_extent, 0.1f, far_plane) * view;
	}

	glm::vec3 target = camera.camera_target;
	float field_of_view = glm::radians(90.0f);
	if (const auto *spot = std::get_if<SpotLightData>(&light.data())) {
		const glm::vec3 direction = safe_direction(light.emissionDirection());
		target = resolved_position + direction * std::max(spot->range, 1.0f);
		field_of_view = std::clamp(spot->outer_cone_radians * 2.0f, glm::radians(1.0f), glm::radians(175.0f));
	}
	const glm::vec3 direction = safe_direction(target - resolved_position);
	const glm::mat4 view = glm::lookAt(
		resolved_position, target, shadow_up_direction(direction));
	return glm::perspective(field_of_view, 1.0f, 0.1f, far_plane) * view;
}
