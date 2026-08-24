#pragma once

#include "editor/model/SpatialOverlayEvidence.h"

#include <vector>

class SpatialBuildingModel;
class SmallModernBuildingModel;

class SpatialOverlayEvidenceService
{
public:
	std::vector<SpatialOverlaySegment> build(
		const SpatialBuildingModel &model,
		const SpatialOverlayEvidenceRequest &request) const;
	std::vector<SpatialOverlaySegment> build(
		const SpatialBuildingModel &model,
		const SmallModernBuildingModel *building_model,
		const SpatialOverlayEvidenceRequest &request) const;
};
