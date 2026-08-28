#include "electrical/service/ElectricalControlValidationService.h"

#include <cmath>
#include <set>

ElectricalValidationReport ElectricalControlValidationService::validate(
	const ElectricalControlGraph &graph) const
{
	ElectricalValidationReport report;
	std::set<std::string> controlled_fixtures;
	for (const LightSwitch &light_switch : graph.switches()) {
		if (!std::isfinite(light_switch.state().dimmer) ||
		    light_switch.state().dimmer < 0.0f || light_switch.state().dimmer > 1.0f) {
			report.addIssue(ElectricalValidationIssue(
				ElectricalValidationCode::InvalidDimmer,
				"P3D-SWITCH-104 INVALID_DIMMER: switch dimmer values must be between zero and one."));
		}
	}
	for (const LightingCircuit &circuit : graph.circuits()) {
		if (circuit.fixtureIds().empty()) {
			report.addIssue(ElectricalValidationIssue(
				ElectricalValidationCode::EmptyCircuit,
				"P3D-CIRCUIT-201 EMPTY_CIRCUIT: each lighting circuit must power at least one fixture."));
		}
		if (!circuit.alwaysOn() && circuit.controlIds().empty()) {
			report.addIssue(ElectricalValidationIssue(
				ElectricalValidationCode::MissingControl,
				"P3D-CIRCUIT-202 MISSING_CONTROL: each non-always-on circuit requires a control."));
		}
		for (const ElectricalObjectId &control_id : circuit.controlIds()) {
			if (graph.findSwitch(control_id) == nullptr) {
				report.addIssue(ElectricalValidationIssue(
					ElectricalValidationCode::InvalidControlTarget,
					"P3D-SWITCH-102 INVALID_CONTROL_TARGET: circuit references an undefined switch."));
			}
		}
		for (const ElectricalObjectId &fixture_id : circuit.fixtureIds()) {
			if (graph.findFixture(fixture_id) == nullptr) {
				report.addIssue(ElectricalValidationIssue(
					ElectricalValidationCode::InvalidFixture,
					"P3D-CIRCUIT-203 INVALID_FIXTURE: circuit references an undefined fixture."));
			} else {
				controlled_fixtures.insert(fixture_id.value());
			}
		}
	}
	for (const LightFixtureObject &fixture : graph.fixtures()) {
		if (controlled_fixtures.count(fixture.objectId().value()) == 0u) {
			report.addIssue(ElectricalValidationIssue(
				ElectricalValidationCode::MissingCircuit,
				"P3D-SWITCH-101 MISSING_CIRCUIT: every powered fixture must belong to a circuit."));
		}
	}
	return report;
}
