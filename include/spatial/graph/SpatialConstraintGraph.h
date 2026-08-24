#pragma once

#include "spatial/model/SpatialConstraint.h"

#include <memory>
#include <utility>
#include <vector>

class SpatialConstraintGraph {
public:
	SpatialConstraintGraph() = default;
	explicit SpatialConstraintGraph(
		std::vector<std::shared_ptr<const SpatialConstraint>> constraints)
		: constraints_(std::move(constraints)) {}

	const std::vector<std::shared_ptr<const SpatialConstraint>> &constraints() const
	{
		return constraints_;
	}

private:
	std::vector<std::shared_ptr<const SpatialConstraint>> constraints_;
};
