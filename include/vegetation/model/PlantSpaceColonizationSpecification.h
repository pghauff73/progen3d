#pragma once

#include "vegetation/model/CrownVolumeSpecification.h"
#include "vegetation/model/SpaceColonizationSpecification.h"
#include "vegetation/model/VegetationObstacleBoundary.h"

#include <utility>
#include <vector>

class PlantSpaceColonizationSpecification
{
public:
	PlantSpaceColonizationSpecification(
		CrownVolumeSpecification crown_volume,
		SpaceColonizationSpecification colonization,
		std::vector<VegetationObstacleBoundary> obstacles = {})
		: crown_volume_(std::move(crown_volume)),
		  colonization_(std::move(colonization)),
		  obstacles_(std::move(obstacles))
	{
	}

	const CrownVolumeSpecification &crownVolume() const
	{
		return crown_volume_;
	}
	const SpaceColonizationSpecification &colonization() const
	{
		return colonization_;
	}
	const std::vector<VegetationObstacleBoundary> &obstacles() const
	{
		return obstacles_;
	}

private:
	CrownVolumeSpecification crown_volume_;
	SpaceColonizationSpecification colonization_;
	std::vector<VegetationObstacleBoundary> obstacles_;
};
