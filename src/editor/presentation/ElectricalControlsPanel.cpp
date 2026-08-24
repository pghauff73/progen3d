#include "editor/presentation/ElectricalControlsPanel.h"

#include "electrical/service/ElectricalControlValidationService.h"
#include "electrical/service/LightingStateEvaluator.h"

#include <imgui.h>

void ElectricalControlsPanel::draw(LightingSceneState *lighting_state) const
{
	if (lighting_state == nullptr) return;
	ElectricalControlGraph &graph = lighting_state->electrical_control_graph;
	ImGui::Checkbox("Activate Controls Mode", &lighting_state->activate_controls_mode);
	ImGui::Text("%zu fixtures, %zu switches, %zu circuits",
	            graph.fixtures().size(), graph.switches().size(), graph.circuits().size());
	bool changed = false;
	ImGui::SeparatorText("Switches");
	if (graph.switches().empty()) ImGui::TextDisabled("No switches are defined in this scene.");
	for (LightSwitch &light_switch : graph.switches()) {
		ImGui::PushID(light_switch.id().value().c_str());
		bool on = light_switch.state().on;
		if (ImGui::Checkbox(light_switch.name().c_str(), &on)) {
			light_switch.state().on = on;
			changed = true;
		}
		if (light_switch.type() == SwitchType::Dimmer || light_switch.type() == SwitchType::Smart ||
		    light_switch.type() == SwitchType::SceneController) {
			float dimmer = light_switch.state().dimmer;
			if (ImGui::SliderFloat("Dimmer", &dimmer, 0.0f, 1.0f, "%.0f%%")) {
				light_switch.state().dimmer = dimmer;
				changed = true;
			}
		}
		ImGui::TextDisabled("Mounted on %s at %.2f m",
		                    light_switch.mountObjectId().empty() ? "<unassigned>" : light_switch.mountObjectId().c_str(),
		                    light_switch.mountHeight());
		ImGui::PopID();
	}
	ImGui::SeparatorText("Circuits");
	if (graph.circuits().empty()) ImGui::TextDisabled("No lighting circuits are defined in this scene.");
	for (const LightingCircuit &circuit : graph.circuits()) {
		ImGui::Text("%s: %zu fixtures, %zu controls%s",
		            circuit.name().c_str(),
		            circuit.fixtureIds().size(),
		            circuit.controlIds().size(),
		            circuit.alwaysOn() ? " (always on)" : "");
	}
	if (changed) LightingStateEvaluator().apply(graph, &lighting_state->lights);
	const ElectricalValidationReport validation = ElectricalControlValidationService().validate(graph);
	if (!validation.isValid()) {
		ImGui::SeparatorText("Validation");
		ImGui::TextWrapped("%s", validation.firstDiagnostic().c_str());
	}
}
