#pragma once

#include "building/model/BuildingServicePortId.h"

#include <cstddef>
#include <utility>

class BuildingServiceContinuityResult {
public:
	BuildingServiceContinuityResult(BuildingServicePortId source_port_id,
	                                BuildingServicePortId target_port_id,
	                                bool reachable,
	                                std::size_t traversed_flow_count)
		: source_port_id_(std::move(source_port_id)),
		  target_port_id_(std::move(target_port_id)),
		  reachable_(reachable),
		  traversed_flow_count_(traversed_flow_count) {}

	const BuildingServicePortId &sourcePortId() const { return source_port_id_; }
	const BuildingServicePortId &targetPortId() const { return target_port_id_; }
	bool reachable() const { return reachable_; }
	std::size_t traversedFlowCount() const { return traversed_flow_count_; }

private:
	BuildingServicePortId source_port_id_;
	BuildingServicePortId target_port_id_;
	bool reachable_ = false;
	std::size_t traversed_flow_count_ = 0;
};
