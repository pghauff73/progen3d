#pragma once

#include "building/model/SmallModernBuildingModelConstructionRequest.h"

class SpatialBuildingModel;

class SmallModernBuildingKnowledgeCatalog {
public:
	bool matchesSpatialModel(const SpatialBuildingModel &spatial_model) const;
	SmallModernBuildingModelConstructionRequest createConstructionRequest() const;
};
