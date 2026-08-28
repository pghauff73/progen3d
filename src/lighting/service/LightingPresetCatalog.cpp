#include "lighting/service/LightingPresetCatalog.h"

#include <glm/gtc/quaternion.hpp>

namespace {

SceneLight directional_light(const char *id,
	                          const char *name,
	                          const glm::vec3 &direction,
	                          float kelvin,
	                          float lux,
	                          bool casts_shadow)
{
	SceneLight light(
		LightId(id), name, SceneLightType::Directional, DirectionalLightData{direction});
	light.setProvenance(LightProvenance::Preset);
	light.emission().setUsesColorTemperature(true);
	light.emission().setColorTemperatureKelvin(kelvin);
	light.emission().setIntensity(lux);
	light.emission().setIntensityUnit(PhotometricIntensityUnit::Lux);
	light.shadow().setEnabled(casts_shadow);
	light.shadow().setResolution(casts_shadow ? 2048u : 1024u);
	return light;
}

SceneLight point_light(const char *id,
	                    const char *name,
	                    const glm::vec3 &position,
	                    float kelvin,
	                    float lumens,
	                    float range,
	                    bool casts_shadow = false)
{
	SceneLight light(
		LightId(id), name, SceneLightType::Point, PointLightData{range});
	light.setPosition(position);
	light.setProvenance(LightProvenance::Preset);
	light.emission().setUsesColorTemperature(true);
	light.emission().setColorTemperatureKelvin(kelvin);
	light.emission().setIntensity(lumens);
	light.emission().setIntensityUnit(PhotometricIntensityUnit::Lumens);
	light.shadow().setEnabled(casts_shadow);
	return light;
}

void add_all(PreviewLightCollection *lights, std::vector<SceneLight> preset_lights)
{
	lights->clear();
	for (SceneLight &light : preset_lights) lights->add(std::move(light));
}

}

const std::vector<LightingPresetDefinition> &LightingPresetCatalog::presets() const
{
	static const std::vector<LightingPresetDefinition> definitions{
		{"Studio", "Camera-relative key, fill, and rim lighting."},
		{"Exterior Day", "Strong daylight with a cool sky fill."},
		{"Sunset", "Low warm sun with subdued cool fill."},
		{"Interior Warm", "Warm residential ceiling and task lights."},
		{"Night", "Cool moonlight with warm occupied-room accents."},
		{"Neutral Evidence", "White balanced scene lights for deterministic placement review."}};
	return definitions;
}

bool LightingPresetCatalog::apply(
	const std::string &preset_name,
	PreviewLightCollection *lights) const
{
	if (lights == nullptr) return false;
	if (preset_name == "Studio") {
		lights->loadStudioDefault();
		return true;
	}
	if (preset_name == "Exterior Day") {
		add_all(lights, {
			directional_light("ExteriorSun", "Exterior Sun", {-0.45f, -0.82f, -0.35f}, 5600.0f, 65000.0f, true),
			directional_light("ExteriorSky", "Exterior Sky Fill", {0.25f, -0.72f, 0.64f}, 8200.0f, 7000.0f, false)});
		return true;
	}
	if (preset_name == "Sunset") {
		add_all(lights, {
			directional_light("SunsetSun", "Sunset Sun", {-0.82f, -0.28f, -0.48f}, 2400.0f, 19000.0f, true),
			directional_light("SunsetSky", "Sunset Sky", {0.32f, -0.76f, 0.56f}, 7200.0f, 2600.0f, false)});
		return true;
	}
	if (preset_name == "Interior Warm") {
		add_all(lights, {
			point_light("LivingCeiling", "Living Ceiling", {0.0f, 3.0f, 0.0f}, 3000.0f, 1400.0f, 10.0f, true),
			point_light("KitchenTask", "Kitchen Task", {3.0f, 2.4f, -1.0f}, 3500.0f, 900.0f, 7.0f),
			point_light("DiningPendant", "Dining Pendant", {-2.5f, 2.2f, 1.0f}, 2700.0f, 800.0f, 7.0f),
			point_light("HallwayWarm", "Hallway Warm", {0.0f, 2.5f, 5.0f}, 3000.0f, 650.0f, 6.0f)});
		return true;
	}
	if (preset_name == "Night") {
		add_all(lights, {
			directional_light("Moon", "Moon", {0.42f, -0.74f, 0.52f}, 9000.0f, 4500.0f, true),
			point_light("NightLiving", "Night Living", {0.0f, 2.8f, 0.0f}, 2700.0f, 650.0f, 8.0f),
			point_light("NightEntry", "Night Entry", {0.0f, 2.0f, -5.0f}, 3000.0f, 420.0f, 5.0f)});
		return true;
	}
	if (preset_name == "Neutral Evidence") {
		lights->loadStudioDefault();
		for (SceneLight &light : lights->lights()) {
			light.emission().setLinearRgb(glm::vec3(1.0f));
			light.emission().setUsesColorTemperature(false);
		}
		return true;
	}
	return false;
}
