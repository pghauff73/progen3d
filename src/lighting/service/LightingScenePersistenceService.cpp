#include "lighting/service/LightingScenePersistenceService.h"

#include <glm/gtc/quaternion.hpp>

#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <system_error>

namespace {

constexpr const char *kLightingSceneHeader = "ProGen3DLightingScene";
constexpr int kLightingSceneVersion = 1;

bool valid_light_type(int value)
{
	return value >= static_cast<int>(SceneLightType::Directional) &&
	       value <= static_cast<int>(SceneLightType::RectangularArea);
}

bool valid_light_provenance(int value)
{
	return value >= static_cast<int>(LightProvenance::Editor) &&
	       value <= static_cast<int>(LightProvenance::Imported);
}

bool valid_intensity_unit(int value)
{
	return value >= static_cast<int>(PhotometricIntensityUnit::Lumens) &&
	       value <= static_cast<int>(PhotometricIntensityUnit::Lux);
}

bool valid_reference_frame(int value)
{
	return value >= static_cast<int>(SceneLightReferenceFrame::World) &&
	       value <= static_cast<int>(SceneLightReferenceFrame::CameraRelative);
}

bool valid_shadow_frequency(int value)
{
	return value >= static_cast<int>(ShadowUpdateFrequency::EveryFrame) &&
	       value <= static_cast<int>(ShadowUpdateFrequency::Static);
}

void light_shape_values(const SceneLight &light,
	                    float *range,
	                    float *inner_cone,
	                    float *outer_cone,
	                    float *area_width,
	                    float *area_height,
	                    bool *two_sided)
{
	*range = light.range();
	*inner_cone = 0.0f;
	*outer_cone = 0.0f;
	*area_width = 0.0f;
	*area_height = 0.0f;
	*two_sided = false;
	if (const auto *spot = std::get_if<SpotLightData>(&light.data())) {
		*inner_cone = spot->inner_cone_radians;
		*outer_cone = spot->outer_cone_radians;
	}
	if (const auto *area = std::get_if<RectangularAreaLightData>(&light.data())) {
		*area_width = area->dimensions.x;
		*area_height = area->dimensions.y;
		*two_sided = area->two_sided;
	}
}

SceneLightData make_light_data(SceneLightType type,
	                           float range,
	                           float inner_cone,
	                           float outer_cone,
	                           float area_width,
	                           float area_height,
	                           bool two_sided)
{
	if (type == SceneLightType::Directional) return DirectionalLightData{};
	if (type == SceneLightType::Spot) {
		return SpotLightData{range, inner_cone, outer_cone};
	}
	if (type == SceneLightType::RectangularArea) {
		return RectangularAreaLightData{
			glm::vec2(area_width, area_height), range, two_sided};
	}
	return PointLightData{range};
}

bool parse_boolean(int value, bool *destination)
{
	if (value != 0 && value != 1) return false;
	*destination = value == 1;
	return true;
}

bool report_parse_error(const std::filesystem::path &path,
	                    std::size_t line_number,
	                    std::string *error_message)
{
	if (error_message != nullptr) {
		*error_message = "Could not parse lighting scene record " + path.string() +
		                 " at line " + std::to_string(line_number) + ".";
	}
	return false;
}

}

std::filesystem::path LightingScenePersistenceService::sidecarPathFor(
	const std::filesystem::path &grammar_path) const
{
	std::filesystem::path sidecar_path = grammar_path;
	sidecar_path += ".lighting.scene";
	return sidecar_path;
}

bool LightingScenePersistenceService::save(
	const std::filesystem::path &grammar_path,
	const LightingSceneState &lighting_state,
	std::string *error_message) const
{
	if (grammar_path.empty()) {
		if (error_message != nullptr) *error_message = "Lighting persistence requires a grammar path.";
		return false;
	}
	const std::filesystem::path sidecar_path = sidecarPathFor(grammar_path);
	std::filesystem::path temporary_path = sidecar_path;
	temporary_path += ".tmp";
	std::ofstream output(temporary_path, std::ios::binary | std::ios::trunc);
	if (!output) {
		if (error_message != nullptr) {
			*error_message = "Could not create lighting scene record: " + temporary_path.string();
		}
		return false;
	}
	output << std::setprecision(std::numeric_limits<float>::max_digits10);
	output << kLightingSceneHeader << ' ' << kLightingSceneVersion << '\n';
	output << "preset " << std::quoted(lighting_state.active_preset_name) << '\n';
	output << "lightGizmos " << (lighting_state.light_gizmos_visible ? 1 : 0) << '\n';
	output << "activateControls " << (lighting_state.activate_controls_mode ? 1 : 0) << '\n';
	for (const SceneLight &light : lighting_state.lights.lights()) {
		if (light.provenance() == LightProvenance::Grammar) continue;
		float range = 0.0f;
		float inner_cone = 0.0f;
		float outer_cone = 0.0f;
		float area_width = 0.0f;
		float area_height = 0.0f;
		bool two_sided = false;
		light_shape_values(
			light, &range, &inner_cone, &outer_cone, &area_width, &area_height, &two_sided);
		const glm::vec3 &position = light.position();
		const glm::quat &orientation = light.orientation();
		const glm::vec3 &color = light.emission().linearRgb();
		output << "light " << std::quoted(light.id().value()) << ' '
		       << std::quoted(light.name()) << ' '
		       << static_cast<int>(light.type()) << ' '
		       << static_cast<int>(light.provenance()) << ' '
		       << (light.enabled() ? 1 : 0) << ' ' << (light.visible() ? 1 : 0) << ' '
		       << position.x << ' ' << position.y << ' ' << position.z << ' '
		       << orientation.w << ' ' << orientation.x << ' ' << orientation.y << ' ' << orientation.z << ' '
		       << (light.emission().usesColorTemperature() ? 1 : 0) << ' '
		       << color.r << ' ' << color.g << ' ' << color.b << ' '
		       << light.emission().colorTemperatureKelvin() << ' '
		       << light.emission().intensity() << ' '
		       << static_cast<int>(light.emission().intensityUnit()) << ' '
		       << light.emission().exposureCompensation() << ' '
		       << light.exposureCompensation() << ' '
		       << light.controlExposureCompensation() << ' '
		       << static_cast<int>(light.referenceFrame()) << ' '
		       << range << ' ' << inner_cone << ' ' << outer_cone << ' '
		       << area_width << ' ' << area_height << ' ' << (two_sided ? 1 : 0) << ' '
		       << (light.shadow().enabled() ? 1 : 0) << ' '
		       << light.shadow().resolution() << ' '
		       << light.shadow().softness() << ' '
		       << light.shadow().constantBias() << ' '
		       << light.shadow().slopeBias() << ' '
		       << light.shadow().normalBias() << ' '
		       << static_cast<int>(light.shadow().updateFrequency()) << '\n';
	}
	for (const LightSwitch &light_switch : lighting_state.electrical_control_graph.switches()) {
		output << "switch " << std::quoted(light_switch.id().value()) << ' '
		       << (light_switch.state().on ? 1 : 0) << ' '
		       << light_switch.state().dimmer << '\n';
	}
	output.flush();
	if (!output) {
		output.close();
		std::error_code remove_error;
		std::filesystem::remove(temporary_path, remove_error);
		if (error_message != nullptr) {
			*error_message = "Could not write lighting scene record: " + sidecar_path.string();
		}
		return false;
	}
	output.close();
	std::error_code remove_error;
	std::filesystem::remove(sidecar_path, remove_error);
	std::error_code rename_error;
	std::filesystem::rename(temporary_path, sidecar_path, rename_error);
	if (rename_error) {
		std::filesystem::remove(temporary_path, remove_error);
		if (error_message != nullptr) {
			*error_message = "Could not replace lighting scene record: " + sidecar_path.string();
		}
		return false;
	}
	return true;
}

bool LightingScenePersistenceService::load(
	const std::filesystem::path &grammar_path,
	LightingSceneState *lighting_state,
	std::string *error_message) const
{
	if (lighting_state == nullptr) {
		if (error_message != nullptr) *error_message = "Lighting persistence requires a scene state.";
		return false;
	}
	lighting_state->resetToStudio();
	const std::filesystem::path sidecar_path = sidecarPathFor(grammar_path);
	if (!std::filesystem::exists(sidecar_path)) return true;
	std::ifstream input(sidecar_path, std::ios::binary);
	if (!input) {
		if (error_message != nullptr) {
			*error_message = "Could not open lighting scene record: " + sidecar_path.string();
		}
		return false;
	}
	std::string header;
	int version = 0;
	if (!(input >> header >> version) || header != kLightingSceneHeader || version != kLightingSceneVersion) {
		return report_parse_error(sidecar_path, 1, error_message);
	}
	std::string remainder;
	std::getline(input, remainder);
	lighting_state->lights.clear();
	lighting_state->electrical_control_graph.clear();
	std::string line;
	std::size_t line_number = 1;
	while (std::getline(input, line)) {
		++line_number;
		if (line.empty()) continue;
		std::istringstream record(line);
		std::string record_type;
		record >> record_type;
		if (record_type == "preset") {
			if (!(record >> std::quoted(lighting_state->active_preset_name))) {
				return report_parse_error(sidecar_path, line_number, error_message);
			}
		}
		else if (record_type == "lightGizmos" || record_type == "activateControls") {
			int value = 0;
			bool parsed_value = false;
			if (!(record >> value) || !parse_boolean(value, &parsed_value)) {
				return report_parse_error(sidecar_path, line_number, error_message);
			}
			if (record_type == "lightGizmos") lighting_state->light_gizmos_visible = parsed_value;
			else lighting_state->activate_controls_mode = parsed_value;
		}
		else if (record_type == "light") {
			std::string id;
			std::string name;
			int type_value = 0;
			int provenance_value = 0;
			int enabled_value = 0;
			int visible_value = 0;
			glm::vec3 position(0.0f);
			glm::quat orientation(1.0f, 0.0f, 0.0f, 0.0f);
			int uses_temperature_value = 0;
			glm::vec3 color(1.0f);
			float kelvin = 6500.0f;
			float intensity = 1000.0f;
			int intensity_unit_value = 0;
			float emission_exposure = 0.0f;
			float exposure = 0.0f;
			float control_exposure = 0.0f;
			int reference_frame_value = 0;
			float range = 10.0f;
			float inner_cone = 0.0f;
			float outer_cone = 0.0f;
			float area_width = 0.0f;
			float area_height = 0.0f;
			int two_sided_value = 0;
			int shadow_enabled_value = 0;
			unsigned int shadow_resolution = 1024;
			float shadow_softness = 1.0f;
			float constant_bias = 0.0005f;
			float slope_bias = 1.5f;
			float normal_bias = 0.0f;
			int shadow_frequency_value = 0;
			if (!(record >> std::quoted(id) >> std::quoted(name)
			      >> type_value >> provenance_value >> enabled_value >> visible_value
			      >> position.x >> position.y >> position.z
			      >> orientation.w >> orientation.x >> orientation.y >> orientation.z
			      >> uses_temperature_value >> color.r >> color.g >> color.b
			      >> kelvin >> intensity >> intensity_unit_value >> emission_exposure
			      >> exposure >> control_exposure >> reference_frame_value
			      >> range >> inner_cone >> outer_cone
			      >> area_width >> area_height >> two_sided_value
			      >> shadow_enabled_value >> shadow_resolution >> shadow_softness
			      >> constant_bias >> slope_bias >> normal_bias >> shadow_frequency_value) ||
			    id.empty() || !valid_light_type(type_value) ||
			    !valid_light_provenance(provenance_value) ||
			    !valid_intensity_unit(intensity_unit_value) ||
			    !valid_reference_frame(reference_frame_value) ||
			    !valid_shadow_frequency(shadow_frequency_value)) {
				return report_parse_error(sidecar_path, line_number, error_message);
			}
			bool enabled = false;
			bool visible = false;
			bool uses_temperature = false;
			bool two_sided = false;
			bool shadow_enabled = false;
			if (!parse_boolean(enabled_value, &enabled) ||
			    !parse_boolean(visible_value, &visible) ||
			    !parse_boolean(uses_temperature_value, &uses_temperature) ||
			    !parse_boolean(two_sided_value, &two_sided) ||
			    !parse_boolean(shadow_enabled_value, &shadow_enabled)) {
				return report_parse_error(sidecar_path, line_number, error_message);
			}
			const SceneLightType type = static_cast<SceneLightType>(type_value);
			SceneLight light(
				LightId(id), name, type,
				make_light_data(type, range, inner_cone, outer_cone,
				                area_width, area_height, two_sided));
			light.setProvenance(static_cast<LightProvenance>(provenance_value));
			light.setEnabled(enabled);
			light.setVisible(visible);
			light.setPosition(position);
			light.setOrientation(orientation);
			light.emission().setUsesColorTemperature(uses_temperature);
			light.emission().setLinearRgb(color);
			light.emission().setColorTemperatureKelvin(kelvin);
			light.emission().setIntensity(intensity);
			light.emission().setIntensityUnit(
				static_cast<PhotometricIntensityUnit>(intensity_unit_value));
			light.emission().setExposureCompensation(emission_exposure);
			light.setExposureCompensation(exposure);
			light.setControlExposureCompensation(control_exposure);
			light.setReferenceFrame(static_cast<SceneLightReferenceFrame>(reference_frame_value));
			light.shadow().setEnabled(shadow_enabled);
			light.shadow().setResolution(shadow_resolution);
			light.shadow().setSoftness(shadow_softness);
			light.shadow().setConstantBias(constant_bias);
			light.shadow().setSlopeBias(slope_bias);
			light.shadow().setNormalBias(normal_bias);
			light.shadow().setUpdateFrequency(
				static_cast<ShadowUpdateFrequency>(shadow_frequency_value));
			lighting_state->lights.add(std::move(light));
		}
		else if (record_type == "switch") {
			std::string id;
			int on_value = 0;
			float dimmer = 1.0f;
			bool on = false;
			if (!(record >> std::quoted(id) >> on_value >> dimmer) || id.empty() ||
			    !parse_boolean(on_value, &on)) {
				return report_parse_error(sidecar_path, line_number, error_message);
			}
			LightSwitch light_switch(ElectricalObjectId(id), id, SwitchType::SinglePole);
			light_switch.state().on = on;
			light_switch.state().dimmer = dimmer;
			if (!lighting_state->electrical_control_graph.addSwitch(std::move(light_switch))) {
				return report_parse_error(sidecar_path, line_number, error_message);
			}
		}
		else {
			return report_parse_error(sidecar_path, line_number, error_message);
		}
		std::string unexpected;
		if (record >> unexpected) return report_parse_error(sidecar_path, line_number, error_message);
	}
	if (lighting_state->lights.lights().empty()) lighting_state->lights.loadStudioDefault();
	lighting_state->selected_light_id = lighting_state->lights.lights().front().id();
	lighting_state->hovered_light_id = LightId();
	return true;
}
