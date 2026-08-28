#pragma once

#include "electrical/model/ElectricalObjectId.h"
#include "electrical/model/LightingCircuitId.h"

#include <string>
#include <utility>
#include <vector>

class LightingCircuit
{
public:
	LightingCircuit() = default;
	LightingCircuit(LightingCircuitId id, std::string name)
		: id_(std::move(id)), name_(std::move(name)) {}

	const LightingCircuitId &id() const { return id_; }
	const std::string &name() const { return name_; }
	void setName(std::string name) { name_ = std::move(name); }
	std::vector<ElectricalObjectId> &fixtureIds() { return fixture_ids_; }
	const std::vector<ElectricalObjectId> &fixtureIds() const { return fixture_ids_; }
	std::vector<ElectricalObjectId> &controlIds() { return control_ids_; }
	const std::vector<ElectricalObjectId> &controlIds() const { return control_ids_; }
	bool alwaysOn() const { return always_on_; }
	void setAlwaysOn(bool always_on) { always_on_ = always_on; }

private:
	LightingCircuitId id_;
	std::string name_;
	std::vector<ElectricalObjectId> fixture_ids_;
	std::vector<ElectricalObjectId> control_ids_;
	bool always_on_ = false;
};
