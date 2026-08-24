#include "electrical/model/ElectricalControlGraph.h"

bool ElectricalControlGraph::addFixture(LightFixtureObject fixture)
{
	if (fixture.objectId().empty() || findFixture(fixture.objectId()) != nullptr) return false;
	fixtures_.push_back(std::move(fixture));
	return true;
}

bool ElectricalControlGraph::addSwitch(LightSwitch light_switch)
{
	if (light_switch.id().empty() || findSwitch(light_switch.id()) != nullptr) return false;
	switches_.push_back(std::move(light_switch));
	return true;
}

bool ElectricalControlGraph::addCircuit(LightingCircuit circuit)
{
	if (circuit.id().empty() || findCircuit(circuit.id()) != nullptr) return false;
	circuits_.push_back(std::move(circuit));
	return true;
}

LightFixtureObject *ElectricalControlGraph::findFixture(const ElectricalObjectId &id)
{
	for (LightFixtureObject &fixture : fixtures_) if (fixture.objectId() == id) return &fixture;
	return nullptr;
}

const LightFixtureObject *ElectricalControlGraph::findFixture(const ElectricalObjectId &id) const
{
	for (const LightFixtureObject &fixture : fixtures_) if (fixture.objectId() == id) return &fixture;
	return nullptr;
}

LightSwitch *ElectricalControlGraph::findSwitch(const ElectricalObjectId &id)
{
	for (LightSwitch &light_switch : switches_) if (light_switch.id() == id) return &light_switch;
	return nullptr;
}

const LightSwitch *ElectricalControlGraph::findSwitch(const ElectricalObjectId &id) const
{
	for (const LightSwitch &light_switch : switches_) if (light_switch.id() == id) return &light_switch;
	return nullptr;
}

LightingCircuit *ElectricalControlGraph::findCircuit(const LightingCircuitId &id)
{
	for (LightingCircuit &circuit : circuits_) if (circuit.id() == id) return &circuit;
	return nullptr;
}

const LightingCircuit *ElectricalControlGraph::findCircuit(const LightingCircuitId &id) const
{
	for (const LightingCircuit &circuit : circuits_) if (circuit.id() == id) return &circuit;
	return nullptr;
}

void ElectricalControlGraph::clear()
{
	fixtures_.clear();
	switches_.clear();
	circuits_.clear();
}
