#pragma once

#include "building/model/SmallModernBuildingModelConstructionRequest.h"
#include "building/model/SmallModernBuildingModelConstructionResult.h"

#include <utility>

class SpatialBuildingModel;

class SmallModernBuildingModelConstructionContext {
public:
	explicit SmallModernBuildingModelConstructionContext(
		SmallModernBuildingModelConstructionRequest request)
		: request_(std::move(request)) {}

	SmallModernBuildingModelConstructionResult finalize(
		const SpatialBuildingModel &spatial_model) const;

private:
	SmallModernBuildingModelConstructionRequest request_;
};
