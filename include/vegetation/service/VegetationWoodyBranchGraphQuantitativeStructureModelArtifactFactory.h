#pragma once

#include "vegetation/model/VegetationMeasuredSourceArtifact.h"
#include "vegetation/model/VegetationWoodyBranchGraphReconstruction.h"

#include <optional>

class VegetationWoodyBranchGraphQuantitativeStructureModelArtifactFactory
{
public:
	std::optional<VegetationMeasuredSourceArtifact> create(
		const VegetationWoodyBranchGraphReconstruction &reconstruction) const;
};
