#pragma once

#include "electrical/model/LightFixtureObject.h"
#include "electrical/model/LightSwitch.h"
#include "electrical/model/LightingCircuit.h"

#include <vector>

class ElectricalControlGraph
{
public:
	bool addFixture(LightFixtureObject fixture);
	bool addSwitch(LightSwitch light_switch);
	bool addCircuit(LightingCircuit circuit);

	LightFixtureObject *findFixture(const ElectricalObjectId &id);
	const LightFixtureObject *findFixture(const ElectricalObjectId &id) const;
	LightSwitch *findSwitch(const ElectricalObjectId &id);
	const LightSwitch *findSwitch(const ElectricalObjectId &id) const;
	LightingCircuit *findCircuit(const LightingCircuitId &id);
	const LightingCircuit *findCircuit(const LightingCircuitId &id) const;

	std::vector<LightFixtureObject> &fixtures() { return fixtures_; }
	const std::vector<LightFixtureObject> &fixtures() const { return fixtures_; }
	std::vector<LightSwitch> &switches() { return switches_; }
	const std::vector<LightSwitch> &switches() const { return switches_; }
	std::vector<LightingCircuit> &circuits() { return circuits_; }
	const std::vector<LightingCircuit> &circuits() const { return circuits_; }

	void clear();

private:
	std::vector<LightFixtureObject> fixtures_;
	std::vector<LightSwitch> switches_;
	std::vector<LightingCircuit> circuits_;
};
