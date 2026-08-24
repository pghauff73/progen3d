#pragma once

#include "building/model/BuildingServiceFlow.h"
#include "building/model/BuildingServicePort.h"
#include "building/model/BuildingServiceSystem.h"

#include <utility>
#include <vector>

class BuildingServiceModel {
public:
	BuildingServiceModel() = default;
	BuildingServiceModel(std::vector<BuildingServiceSystem> systems,
	                     std::vector<BuildingServicePort> ports,
	                     std::vector<BuildingServiceFlow> flows)
		: systems_(std::move(systems)),
		  ports_(std::move(ports)),
		  flows_(std::move(flows)) {}

	const std::vector<BuildingServiceSystem> &systems() const { return systems_; }
	const std::vector<BuildingServicePort> &ports() const { return ports_; }
	const std::vector<BuildingServiceFlow> &flows() const { return flows_; }

	const BuildingServicePort *findPort(const BuildingServicePortId &port_id) const
	{
		for (const BuildingServicePort &port : ports_) {
			if (port.portId() == port_id) return &port;
		}
		return nullptr;
	}

	const BuildingServiceSystem *findSystem(const BuildingServiceSystemId &system_id) const
	{
		for (const BuildingServiceSystem &system : systems_) {
			if (system.systemId() == system_id) return &system;
		}
		return nullptr;
	}

private:
	std::vector<BuildingServiceSystem> systems_;
	std::vector<BuildingServicePort> ports_;
	std::vector<BuildingServiceFlow> flows_;
};
