#include "lighting/service/GpuLightRecordFactory.h"

#include "lighting/service/PhotometricColorService.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr std::uint32_t kEnabledFlag = 1u << 0u;
constexpr std::uint32_t kShadowFlag = 1u << 1u;
constexpr std::uint32_t kTwoSidedFlag = 1u << 2u;
constexpr std::uint32_t kCameraRelativeFlag = 1u << 3u;

float smooth_range_cutoff(float distance, float range)
{
	if (!std::isfinite(range) || range <= 0.0f || distance >= range) return 0.0f;
	const float normalized_distance = distance / range;
	const float transition = std::clamp((normalized_distance - 0.8f) / 0.2f, 0.0f, 1.0f);
	const float smooth = transition * transition * (3.0f - 2.0f * transition);
	const float remaining = 1.0f - smooth;
	return remaining * remaining;
}

}

GpuLightRecord GpuLightRecordFactory::make(
	const SceneLight &light,
	const PreviewLightCameraContext &camera,
	const ShadowAssignment &shadow_assignment) const
{
	GpuLightRecord record;
	const glm::vec3 resolved_position = resolvePosition(light, camera);
	const glm::vec3 resolved_color = light.emission().usesColorTemperature()
		? PhotometricColorService().kelvinToLinearRgb(
			light.emission().colorTemperatureKelvin())
		: light.emission().linearRgb();
	const glm::vec3 direction = light.emissionDirection();
	const float range = std::max(light.range(), 0.0f);
	record.position_and_type = glm::vec4(
		resolved_position, static_cast<float>(static_cast<std::uint32_t>(light.type())));
	record.direction_and_range = glm::vec4(direction, range);
	record.color_and_intensity = glm::vec4(
		glm::max(resolved_color, glm::vec3(0.0f)),
		convertIntensity(light, camera, resolved_position));

	if (const auto *spot = std::get_if<SpotLightData>(&light.data())) {
		record.cone_and_area.x = std::cos(spot->inner_cone_radians);
		record.cone_and_area.y = std::cos(spot->outer_cone_radians);
	}
	if (const auto *area = std::get_if<RectangularAreaLightData>(&light.data())) {
		record.cone_and_area.z = area->dimensions.x;
		record.cone_and_area.w = area->dimensions.y;
	}

	std::uint32_t flags = light.enabled() ? kEnabledFlag : 0u;
	if (light.shadow().enabled() && shadow_assignment.shadow_index >= 0) flags |= kShadowFlag;
	if (const auto *area = std::get_if<RectangularAreaLightData>(&light.data())) {
		if (area->two_sided) flags |= kTwoSidedFlag;
	}
	if (light.referenceFrame() == SceneLightReferenceFrame::CameraRelative) {
		flags |= kCameraRelativeFlag;
	}
	record.metadata = glm::uvec4(
		flags,
		shadow_assignment.shadow_index >= 0
			? static_cast<std::uint32_t>(shadow_assignment.shadow_index)
			: 0xffffffffu,
		static_cast<std::uint32_t>(light.provenance()),
		0u);
	return record;
}

glm::vec3 GpuLightRecordFactory::resolvePosition(
	const SceneLight &light,
	const PreviewLightCameraContext &camera) const
{
	if (light.referenceFrame() == SceneLightReferenceFrame::World) {
		return light.position() * camera.scene_scale;
	}
	const float light_distance = std::max(
		7.5f * camera.scene_scale, camera.orbit_distance * 1.35f);
	return camera.camera_target +
		(camera.camera_right * light.position().x +
		 camera.camera_up * light.position().y +
		 camera.camera_view_direction * light.position().z) * light_distance;
}

float GpuLightRecordFactory::convertIntensity(
	const SceneLight &light,
	const PreviewLightCameraContext &camera,
	const glm::vec3 &resolved_position) const
{
	float intensity = std::max(light.emission().intensity(), 0.0f);
	switch (light.emission().intensityUnit()) {
	case PhotometricIntensityUnit::Lumens:
		if (light.type() == SceneLightType::Spot) {
			const auto *spot = std::get_if<SpotLightData>(&light.data());
			const float outer = spot != nullptr ? spot->outer_cone_radians : 0.55f;
			const float solid_angle = std::max(2.0f * 3.14159265358979323846f *
				(1.0f - std::cos(outer)), 0.001f);
			intensity /= solid_angle;
		} else {
			intensity /= 4.0f * 3.14159265358979323846f;
		}
		break;
	case PhotometricIntensityUnit::Candela:
		break;
	case PhotometricIntensityUnit::Lux:
		intensity *= 0.0001f;
		break;
	}
	intensity *= std::exp2(
		light.emission().exposureCompensation() + light.effectiveExposureCompensation());

	if (light.referenceFrame() == SceneLightReferenceFrame::CameraRelative &&
	    light.type() != SceneLightType::Directional) {
		const float target_distance = std::max(
			glm::length(resolved_position - camera.camera_target), 0.001f);
		const float legacy_attenuation =
			1.0f / (1.0f + target_distance * target_distance * 0.018f);
		const float cutoff = std::max(
			smooth_range_cutoff(target_distance, std::max(light.range(), target_distance * 2.0f)),
			0.001f);
		intensity *= legacy_attenuation * target_distance * target_distance / cutoff;
	}
	return intensity;
}
