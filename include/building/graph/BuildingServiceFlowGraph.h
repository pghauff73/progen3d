#pragma once

#include "building/model/BuildingServiceFlow.h"

#include <utility>
#include <vector>

class BuildingServiceFlowGraph {
public:
	BuildingServiceFlowGraph() = default;
	explicit BuildingServiceFlowGraph(std::vector<BuildingServiceFlow> flows)
		: flows_(std::move(flows)) {}

	const std::vector<BuildingServiceFlow> &flows() const { return flows_; }

private:
	std::vector<BuildingServiceFlow> flows_;
};
