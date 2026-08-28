#include "lighting/model/PreviewLightCollection.h"

#include <string>

namespace {

SceneLight make_studio_light(const char *id,
	                          const char *name,
	                          const glm::vec3 &camera_relative_position,
	                          const glm::vec3 &color,
	                          float intensity,
	                          bool casts_shadow)
{
	SceneLight light(
		LightId(id), name, SceneLightType::Point, PointLightData{100.0f});
	light.setPosition(camera_relative_position);
	light.setReferenceFrame(SceneLightReferenceFrame::CameraRelative);
	light.setProvenance(LightProvenance::Preset);
	light.emission().setLinearRgb(color);
	light.emission().setIntensity(intensity);
	light.emission().setIntensityUnit(PhotometricIntensityUnit::Candela);
	light.shadow().setEnabled(casts_shadow);
	light.shadow().setResolution(casts_shadow ? 2048u : 1024u);
	return light;
}

}

LightId PreviewLightCollection::add(SceneLight light)
{
	const LightId assigned_id = uniqueIdFor(light.id());
	light.setId(assigned_id);
	lights_.push_back(std::move(light));
	return assigned_id;
}

bool PreviewLightCollection::remove(const LightId &id)
{
	for (auto iterator = lights_.begin(); iterator != lights_.end(); ++iterator) {
		if (iterator->id() == id) {
			lights_.erase(iterator);
			return true;
		}
	}
	return false;
}

SceneLight *PreviewLightCollection::find(const LightId &id)
{
	for (SceneLight &light : lights_) {
		if (light.id() == id) return &light;
	}
	return nullptr;
}

const SceneLight *PreviewLightCollection::find(const LightId &id) const
{
	for (const SceneLight &light : lights_) {
		if (light.id() == id) return &light;
	}
	return nullptr;
}

void PreviewLightCollection::clear()
{
	lights_.clear();
}

void PreviewLightCollection::loadStudioDefault()
{
	clear();
	add(make_studio_light(
		"StudioKey", "Studio Key", glm::vec3(0.74f, 0.88f, -0.62f),
		glm::vec3(1.00f, 0.95f, 0.88f), 1.58f, true));
	add(make_studio_light(
		"StudioFill", "Studio Fill", glm::vec3(-1.06f, 0.18f, 0.28f),
		glm::vec3(0.74f, 0.83f, 1.00f), 0.72f, false));
	add(make_studio_light(
		"StudioRim", "Studio Rim", glm::vec3(0.94f, 0.56f, 0.96f),
		glm::vec3(0.98f, 0.99f, 1.00f), 1.02f, false));
}

void PreviewLightCollection::replaceGrammarLights(
	const std::vector<SceneLight> &grammar_lights)
{
	std::vector<SceneLight> retained_lights;
	retained_lights.reserve(lights_.size() + grammar_lights.size());
	for (const SceneLight &light : lights_) {
		if (light.provenance() != LightProvenance::Grammar) {
			retained_lights.push_back(light);
		}
	}
	lights_ = std::move(retained_lights);
	for (const SceneLight &light : grammar_lights) add(light);
}

LightId PreviewLightCollection::uniqueIdFor(const LightId &requested_id) const
{
	const std::string base = requested_id.empty() ? "Light" : requested_id.value();
	LightId candidate(base);
	if (find(candidate) == nullptr) return candidate;
	for (std::size_t suffix = 2; ; ++suffix) {
		candidate = LightId(base + std::to_string(suffix));
		if (find(candidate) == nullptr) return candidate;
	}
}
