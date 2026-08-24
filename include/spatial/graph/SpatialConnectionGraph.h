#pragma once

#include "spatial/model/SpatialConnection.h"

#include <utility>
#include <vector>

class SpatialConnectionGraph {
public:
	SpatialConnectionGraph() = default;
	explicit SpatialConnectionGraph(std::vector<SpatialConnection> connections)
		: connections_(std::move(connections)) {}

	const std::vector<SpatialConnection> &connections() const { return connections_; }

private:
	std::vector<SpatialConnection> connections_;
};
