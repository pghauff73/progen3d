#include "electrical/service/LightingStateEvaluator.h"

#include <algorithm>
#include <cmath>

void LightingStateEvaluator::apply(
	const ElectricalControlGraph &control_graph,
	PreviewLightCollection *lights) const
{
	if (lights == nullptr) return;
	for (const LightingCircuit &circuit : control_graph.circuits()) {
		bool circuit_on = circuit.alwaysOn();
		float circuit_dimmer = circuit.alwaysOn() ? 1.0f : 0.0f;
		for (const ElectricalObjectId &control_id : circuit.controlIds()) {
			const LightSwitch *light_switch = control_graph.findSwitch(control_id);
			if (light_switch == nullptr) continue;
			if (light_switch->state().on) {
				circuit_on = true;
				circuit_dimmer = std::max(
					circuit_dimmer, std::clamp(light_switch->state().dimmer, 0.0f, 1.0f));
			}
		}
		for (const ElectricalObjectId &fixture_id : circuit.fixtureIds()) {
			const LightFixtureObject *fixture = control_graph.findFixture(fixture_id);
			if (fixture == nullptr) continue;
			for (const LightEmitterDefinition &emitter : fixture->emitters()) {
				SceneLight *light = lights->find(emitter.scene_light_id);
				if (light == nullptr) continue;
				light->setEnabled(circuit_on);
				light->setControlExposureCompensation(
					circuit_on && circuit_dimmer > 0.0f
						? std::log2(circuit_dimmer)
						: -24.0f);
			}
		}
	}
}
