#include "editor/presentation/ElectricalControlsPanel.h"

#include "electrical/service/ElectricalControlValidationService.h"
#include "electrical/service/LightingStateEvaluator.h"

#include "electrical/service/ElectricalFieldEvaluator.h"

#include <imgui.h>

namespace {
void drawMachineFields(const ElectricalMachineModel &scene) {
    if (scene.wires.empty() && scene.coils.empty() && scene.magnets.empty() && scene.motors.empty()) return;
    ImGui::SeparatorText("Electrical machines");
    for (const auto &[id, wire] : scene.wires) {
        if (!ImGui::TreeNode(id.c_str(), "%s | magnet wire", id.c_str())) continue;
        ImGui::Text("%s, %.3g mm bare diameter, %.3g C", wire.conductor->name, wire.diameter * 1000, wire.temperature);
        ImGui::Text("%s, %.3g um insulation, relative permittivity %.3g", wire.insulation->name,
                    wire.insulationThickness * 1e6, wire.insulation->relativePermittivity);
        ImGui::Text("Resistance per metre %.5g ohm/m", wire.resistancePerMetre());
        ImGui::TreePop();
    }
    for (const auto &[id, coil] : scene.coils) {
        if (!ImGui::TreeNode(id.c_str(), "%s | field coil", id.c_str())) continue;
        const auto point = ElectricalFieldEvaluator::winding(coil, scene.wires.at(coil.wireId));
        ImGui::Text("%s wire, %s core, %u turns, %.4g A", coil.wireId.c_str(), coil.core->name, coil.turns, coil.current);
        ImGui::Text("R %.5g ohm | L %.5g H | instantaneous loss %.5g W", point.resistance, point.inductance, point.ohmicPower);
        ImGui::Text("Conductor E %.5g V/m | J %.5g A/m^2", point.conductorField, point.currentDensity);
        ImGui::Text("Insulation E %.5g V/m | D %.5g C/m^2", point.insulationField, point.displacement);
        ImGui::Text("B %.5g T | H(core) %.5g A/m | flux %.5g Wb", point.fluxDensity, point.coreField, point.flux);
        if (coil.gap > 0) ImGui::Text("H(gap) %.5g A/m", point.gapField);
        ImGui::Text("Stored field energy %.5g J", point.storedEnergy);
        if (point.beyondCoreLimit) ImGui::TextWrapped("Core exceeds the linear model range.");
        ImGui::TreePop();
    }
    for (const auto &[id, magnet] : scene.magnets) {
        if (!ImGui::TreeNode(id.c_str(), "%s | permanent magnet", id.c_str())) continue;
        const auto point = ElectricalFieldEvaluator::magnet(magnet);
        ImGui::Text("%s | return core %s | polarity %+d", magnet.material->name, magnet.returnCore->name, magnet.polarity);
        ImGui::Text("Br %.4g T | B %.5g T | H(magnet) %.5g A/m", magnet.material->remanence, point.fluxDensity, point.magnetField);
        ImGui::Text("Flux per pole %.5g Wb", point.flux);
        if (point.beyondCoreLimit) ImGui::TextWrapped("Return core exceeds the linear model range.");
        ImGui::TreePop();
    }
    for (const auto &[id, motor] : scene.motors) {
        if (!ImGui::TreeNode(id.c_str(), "%s | motor", id.c_str())) continue;
        const auto point = ElectricalFieldEvaluator::motor(motor, scene);
        ImGui::Text("%s | %s", motorTopologyName(motor.topology), windingConnectionName(motor.connection));
        ImGui::Text("%u pole pairs | %.4g rpm | winding %s", motor.polePairs, motor.rpm, motor.windingId.c_str());
        ImGui::Text("Torque %.5g N m | converted power %.5g W", point.torque, point.convertedPower);
        ImGui::Text("Winding loss %.5g W | line current %.5g A | %.5g Hz", point.windingLoss, point.lineCurrent, point.electricalFrequency);
        if (motor.cage) {
            ImGui::Text("%u %s cage paths | synchronous speed %.5g rpm", motor.cage->bars, motor.cage->conductor->name, *point.synchronousRpm);
            ImGui::Text("Slip %.4g %% | rotor %.5g Hz | rotor loss %.5g W", *point.slip * 100, *point.rotorFrequency, *point.rotorLoss);
        } else {
            ImGui::Text("Magnet %s | line back EMF %.5g V %s", motor.magnetId.c_str(), *point.lineBackEmf,
                        motor.topology == MotorTopology::SixStepOutrunner ? "active pair" :
                        motor.topology == MotorTopology::BrushedDC ? "DC" : "RMS");
        }
        if (point.beyondCoreLimit) ImGui::TextWrapped("One or more core estimates exceed the linear range.");
        ImGui::TreePop();
    }
}
}


void ElectricalControlsPanel::draw(LightingSceneState *lighting_state) const
{
	if (lighting_state == nullptr) return;
	ElectricalControlGraph &graph = lighting_state->electrical_control_graph;
	ImGui::Checkbox("Activate Controls Mode", &lighting_state->activate_controls_mode);
	ImGui::Text("%zu fixtures, %zu switches, %zu circuits",
	            graph.fixtures().size(), graph.switches().size(), graph.circuits().size());
	drawMachineFields(graph.machines());
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
