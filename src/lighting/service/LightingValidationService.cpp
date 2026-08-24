#include "lighting/service/LightingValidationService.h"

#include <cmath>
#include <set>

namespace {

bool finite_vec3(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool valid_shadow_resolution(std::uint32_t resolution)
{
	return resolution == 256u || resolution == 512u || resolution == 1024u ||
	       resolution == 2048u || resolution == 4096u;
}

}

LightingValidationReport LightingValidationService::validate(
	const PreviewLightCollection &lights) const
{
	LightingValidationReport report;
	std::set<std::string> identifiers;
	if (lights.lights().size() > PreviewLightCollection::maximum_visible_lights) {
		report.addIssue(LightingValidationIssue(
			LightingValidationCode::LightLimitExceeded,
			LightId(),
			"P3D-LIGHT-008 LIGHT_LIMIT_EXCEEDED: preview lighting supports at most 32 visible lights."));
	}
	for (const SceneLight &light : lights.lights()) {
		if (light.id().empty() || !identifiers.insert(light.id().value()).second) {
			report.addIssue(LightingValidationIssue(
				LightingValidationCode::DuplicateLightId,
				light.id(),
				"P3D-LIGHT-009 DUPLICATE_LIGHT_ID: light identifiers must be non-empty and unique."));
		}
		if (!finite_vec3(light.position()) || !finite_vec3(light.emissionDirection())) {
			report.addIssue(LightingValidationIssue(
				LightingValidationCode::NonfiniteTransform,
				light.id(),
				"P3D-LIGHT-002 NONFINITE_TRANSFORM: light position and direction must be finite."));
		}
		if (!std::isfinite(light.emission().intensity()) || light.emission().intensity() < 0.0f) {
			report.addIssue(LightingValidationIssue(
				LightingValidationCode::InvalidIntensity,
				light.id(),
				"P3D-LIGHT-003 INVALID_INTENSITY: light intensity must be finite and non-negative."));
		}
		if (light.type() != SceneLightType::Directional &&
		    (!std::isfinite(light.range()) || light.range() <= 0.0f)) {
			report.addIssue(LightingValidationIssue(
				LightingValidationCode::InvalidRange,
				light.id(),
				"P3D-LIGHT-004 INVALID_RANGE: point, spot, and area light ranges must be positive."));
		}
		if (const auto *spot = std::get_if<SpotLightData>(&light.data())) {
			if (!std::isfinite(spot->inner_cone_radians) ||
			    !std::isfinite(spot->outer_cone_radians) ||
			    spot->inner_cone_radians < 0.0f ||
			    spot->inner_cone_radians > spot->outer_cone_radians ||
			    spot->outer_cone_radians >= 1.57079632679f) {
				report.addIssue(LightingValidationIssue(
					LightingValidationCode::InvalidSpotCone,
					light.id(),
					"P3D-LIGHT-005 INVALID_SPOT_CONE: inner cone must not exceed outer cone and both must be below 90 degrees."));
			}
		}
		if (const auto *area = std::get_if<RectangularAreaLightData>(&light.data())) {
			if (!std::isfinite(area->dimensions.x) || !std::isfinite(area->dimensions.y) ||
			    area->dimensions.x <= 0.0f || area->dimensions.y <= 0.0f) {
				report.addIssue(LightingValidationIssue(
					LightingValidationCode::InvalidAreaSize,
					light.id(),
					"P3D-LIGHT-006 INVALID_AREA_SIZE: rectangular area dimensions must be finite and positive."));
			}
		}
		if (light.shadow().enabled() && !valid_shadow_resolution(light.shadow().resolution())) {
			report.addIssue(LightingValidationIssue(
				LightingValidationCode::InvalidShadowResolution,
				light.id(),
				"P3D-LIGHT-007 SHADOW_ALLOCATION_FAILED: shadow resolution must be 256, 512, 1024, 2048, or 4096."));
		}
	}
	return report;
}
