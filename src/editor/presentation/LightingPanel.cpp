#include "editor/presentation/LightingPanel.h"

#include "lighting/service/LightingEvidenceHashService.h"
#include "lighting/service/LightingPresetCatalog.h"
#include "lighting/service/LightingValidationService.h"

#include <imgui.h>

#include <algorithm>
#include <cstdio>

namespace {

const char *light_type_label(SceneLightType type)
{
	switch (type) {
	case SceneLightType::Directional: return "Directional";
	case SceneLightType::Point: return "Point";
	case SceneLightType::Spot: return "Spot";
	case SceneLightType::RectangularArea: return "Rectangular Area";
	}
	return "Unknown";
}

const char *provenance_label(LightProvenance provenance)
{
	switch (provenance) {
	case LightProvenance::Grammar: return "Grammar";
	case LightProvenance::Editor: return "Editor";
	case LightProvenance::Preset: return "Preset";
	case LightProvenance::Imported: return "Imported";
	}
	return "Unknown";
}

SceneLight new_light(SceneLightType type, std::size_t sequence)
{
	const std::string id = "EditorLight" + std::to_string(sequence);
	SceneLight light(LightId(id), id, type, PointLightData{10.0f});
	light.setType(type);
	light.setPosition(glm::vec3(0.0f, 3.0f, 0.0f));
	light.setProvenance(LightProvenance::Editor);
	light.emission().setIntensityUnit(PhotometricIntensityUnit::Lumens);
	light.emission().setIntensity(type == SceneLightType::Directional ? 30000.0f : 900.0f);
	if (type == SceneLightType::Directional) {
		light.emission().setIntensityUnit(PhotometricIntensityUnit::Lux);
		light.setEmissionDirection(glm::vec3(-0.4f, -0.8f, -0.3f));
	}
	return light;
}

void draw_light_properties(SceneLight *light)
{
	if (light == nullptr) return;
	ImGui::SeparatorText(light->name().c_str());
	ImGui::Text("ID: %s", light->id().value().c_str());
	ImGui::Text("Source: %s", provenance_label(light->provenance()));
	bool enabled = light->enabled();
	if (ImGui::Checkbox("Enabled", &enabled)) light->setEnabled(enabled);
	ImGui::SameLine();
	bool visible = light->visible();
	if (ImGui::Checkbox("Gizmo Visible", &visible)) light->setVisible(visible);

	int type_index = static_cast<int>(light->type());
	const char *type_labels[] = {"Directional", "Point", "Spot", "Rectangular Area"};
	if (ImGui::Combo("Type", &type_index, type_labels, 4)) {
		light->setType(static_cast<SceneLightType>(type_index));
	}
	glm::vec3 position = light->position();
	if (ImGui::DragFloat3("Position", &position.x, 0.05f)) light->setPosition(position);
	glm::vec3 direction = light->emissionDirection();
	if (ImGui::DragFloat3("Direction", &direction.x, 0.02f, -1.0f, 1.0f)) {
		light->setEmissionDirection(direction);
	}

	bool use_temperature = light->emission().usesColorTemperature();
	if (ImGui::Checkbox("Use Color Temperature", &use_temperature)) {
		light->emission().setUsesColorTemperature(use_temperature);
	}
	if (use_temperature) {
		float kelvin = light->emission().colorTemperatureKelvin();
		if (ImGui::SliderFloat("Temperature", &kelvin, 1000.0f, 12000.0f, "%.0f K")) {
			light->emission().setColorTemperatureKelvin(kelvin);
		}
	} else {
		glm::vec3 color = light->emission().linearRgb();
		if (ImGui::ColorEdit3("Linear Color", &color.x, ImGuiColorEditFlags_Float)) {
			light->emission().setLinearRgb(color);
		}
	}
	float intensity = light->emission().intensity();
	if (ImGui::DragFloat("Intensity", &intensity, 1.0f, 0.0f, 100000.0f, "%.2f")) {
		light->emission().setIntensity(intensity);
	}
	int unit_index = static_cast<int>(light->emission().intensityUnit());
	const char *unit_labels[] = {"Lumens", "Candela", "Lux"};
	if (ImGui::Combo("Photometric Unit", &unit_index, unit_labels, 3)) {
		light->emission().setIntensityUnit(static_cast<PhotometricIntensityUnit>(unit_index));
	}
	float exposure = light->exposureCompensation();
	if (ImGui::SliderFloat("Exposure Compensation", &exposure, -8.0f, 8.0f, "%+.2f EV")) {
		light->setExposureCompensation(exposure);
	}

	if (auto *point = std::get_if<PointLightData>(&light->data())) {
		ImGui::DragFloat("Range", &point->range, 0.1f, 0.1f, 1000.0f, "%.2f m");
	}
	if (auto *spot = std::get_if<SpotLightData>(&light->data())) {
		ImGui::DragFloat("Range", &spot->range, 0.1f, 0.1f, 1000.0f, "%.2f m");
		float inner_degrees = glm::degrees(spot->inner_cone_radians);
		float outer_degrees = glm::degrees(spot->outer_cone_radians);
		if (ImGui::SliderFloat("Inner Cone", &inner_degrees, 0.0f, 89.0f, "%.1f deg")) {
			spot->inner_cone_radians = glm::radians(std::min(inner_degrees, outer_degrees));
		}
		if (ImGui::SliderFloat("Outer Cone", &outer_degrees, 0.1f, 89.0f, "%.1f deg")) {
			spot->outer_cone_radians = glm::radians(std::max(outer_degrees, inner_degrees));
		}
	}
	if (auto *area = std::get_if<RectangularAreaLightData>(&light->data())) {
		ImGui::DragFloat2("Area Size", &area->dimensions.x, 0.05f, 0.01f, 100.0f, "%.2f m");
		ImGui::DragFloat("Range", &area->range, 0.1f, 0.1f, 1000.0f, "%.2f m");
		ImGui::Checkbox("Two Sided", &area->two_sided);
	}

	bool casts_shadow = light->shadow().enabled();
	if (ImGui::Checkbox("Cast Shadow", &casts_shadow)) light->shadow().setEnabled(casts_shadow);
	if (casts_shadow) {
		int resolution = static_cast<int>(light->shadow().resolution());
		const char *resolution_labels[] = {"256", "512", "1024", "2048", "4096"};
		const int resolution_values[] = {256, 512, 1024, 2048, 4096};
		int resolution_index = 2;
		for (int index = 0; index < 5; ++index) {
			if (resolution_values[index] == resolution) resolution_index = index;
		}
		if (ImGui::Combo("Shadow Resolution", &resolution_index, resolution_labels, 5)) {
			light->shadow().setResolution(static_cast<unsigned int>(resolution_values[resolution_index]));
		}
		float softness = light->shadow().softness();
		if (ImGui::SliderFloat("Shadow Softness", &softness, 0.0f, 8.0f)) {
			light->shadow().setSoftness(softness);
		}
	}
}

}

void LightingPanel::draw(LightingSceneState *lighting_state) const
{
	if (lighting_state == nullptr) return;
	LightingPresetCatalog preset_catalog;
	const auto &presets = preset_catalog.presets();
	int selected_preset = -1;
	for (std::size_t index = 0; index < presets.size(); ++index) {
		if (presets[index].name() == lighting_state->active_preset_name) {
			selected_preset = static_cast<int>(index);
		}
	}
	std::vector<const char *> preset_names;
	for (const LightingPresetDefinition &preset : presets) preset_names.push_back(preset.name().c_str());
	const char *preset_preview = selected_preset >= 0
		? preset_names[static_cast<std::size_t>(selected_preset)]
		: lighting_state->active_preset_name.c_str();
	if (ImGui::BeginCombo("Lighting Preset", preset_preview)) {
		for (std::size_t index = 0; index < presets.size(); ++index) {
			const bool selected = selected_preset == static_cast<int>(index);
			if (ImGui::Selectable(presets[index].name().c_str(), selected)) {
				selected_preset = static_cast<int>(index);
				lighting_state->active_preset_name = presets[index].name();
				preset_catalog.apply(lighting_state->active_preset_name, &lighting_state->lights);
				lighting_state->selected_light_id = lighting_state->lights.lights().empty()
					? LightId()
					: lighting_state->lights.lights().front().id();
			}
			if (selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}
	if (selected_preset >= 0) {
		ImGui::TextWrapped("%s", presets[static_cast<std::size_t>(selected_preset)].description().c_str());
	} else {
		ImGui::TextWrapped("Custom scene lights are stored beside the grammar document.");
	}
	ImGui::Checkbox("Show Light Gizmos", &lighting_state->light_gizmos_visible);

	if (ImGui::Button("+ Point")) {
		lighting_state->selected_light_id = lighting_state->lights.add(
			new_light(SceneLightType::Point, lighting_state->lights.lights().size() + 1u));
		lighting_state->active_preset_name = "Custom";
	}
	ImGui::SameLine();
	if (ImGui::Button("+ Spot")) {
		lighting_state->selected_light_id = lighting_state->lights.add(
			new_light(SceneLightType::Spot, lighting_state->lights.lights().size() + 1u));
		lighting_state->active_preset_name = "Custom";
	}
	ImGui::SameLine();
	if (ImGui::Button("+ Directional")) {
		lighting_state->selected_light_id = lighting_state->lights.add(
			new_light(SceneLightType::Directional, lighting_state->lights.lights().size() + 1u));
		lighting_state->active_preset_name = "Custom";
	}
	ImGui::SameLine();
	if (ImGui::Button("+ Area")) {
		lighting_state->selected_light_id = lighting_state->lights.add(
			new_light(SceneLightType::RectangularArea, lighting_state->lights.lights().size() + 1u));
		lighting_state->active_preset_name = "Custom";
	}

	ImGui::SeparatorText("Scene Lights");
	for (const SceneLight &light : lighting_state->lights.lights()) {
		const bool selected = light.id() == lighting_state->selected_light_id;
		const std::string label = light.name() + "  [" + light_type_label(light.type()) + "]##" + light.id().value();
		if (ImGui::Selectable(label.c_str(), selected)) lighting_state->selected_light_id = light.id();
	}
	SceneLight *selected_light = lighting_state->lights.find(lighting_state->selected_light_id);
	if (selected_light != nullptr) {
		draw_light_properties(selected_light);
		if (selected_light->provenance() != LightProvenance::Grammar && ImGui::Button("Remove Selected Light")) {
			lighting_state->lights.remove(selected_light->id());
			lighting_state->selected_light_id = LightId();
			lighting_state->active_preset_name = "Custom";
		}
	}

	const LightingValidationReport validation = LightingValidationService().validate(lighting_state->lights);
	lighting_state->evidence = LightingEvidenceHashService().calculate(
		lighting_state->lights,
		lighting_state->electrical_control_graph.fixtures().size(),
		lighting_state->electrical_control_graph.circuits().size(),
		lighting_state->active_preset_name);
	ImGui::SeparatorText("Evidence");
	ImGui::Text("Lights: %zu scene, %zu enabled, %zu GPU",
	            lighting_state->evidence.scene_light_count,
	            lighting_state->evidence.enabled_light_count,
	            lighting_state->evidence.gpu_light_record_count);
	ImGui::Text("SSBO upload: %zu bytes", lighting_state->evidence.lighting_upload_bytes);
	ImGui::Text("Shadow budget: %zu assigned, %zu deferred",
	            lighting_state->evidence.shadow_budget_assignment_count,
	            lighting_state->evidence.deferred_shadow_count);
	ImGui::Text("Lighting path: %s",
	            lighting_state->evidence.forward_plus_recommended
		            ? "Bounded Forward (Forward+ recommended)"
		            : "Bounded Forward SSBO");
	ImGui::Text("State hash: %016llx",
	            static_cast<unsigned long long>(lighting_state->evidence.state_hash));
	if (!validation.isValid()) ImGui::TextWrapped("Validation: %s", validation.firstDiagnostic().c_str());
}
