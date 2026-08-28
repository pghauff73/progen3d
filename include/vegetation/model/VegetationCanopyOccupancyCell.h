#pragma once

#include "vegetation/model/VegetationCanopyVoxelIndex.h"
#include "vegetation/model/VegetationMeasuredPoint3d.h"

#include <cstddef>

class VegetationCanopyOccupancyCell
{
public:
	VegetationCanopyOccupancyCell(
		VegetationCanopyVoxelIndex index,
		VegetationMeasuredPoint3d centre_metres,
		std::size_t foliage_point_count,
		std::size_t woody_point_count,
		std::size_t unknown_point_count)
		: index_(index),
		  centre_metres_(centre_metres),
		  foliage_point_count_(foliage_point_count),
		  woody_point_count_(woody_point_count),
		  unknown_point_count_(unknown_point_count)
	{
	}

	const VegetationCanopyVoxelIndex &index() const { return index_; }
	const VegetationMeasuredPoint3d &centreMetres() const
	{
		return centre_metres_;
	}
	std::size_t foliagePointCount() const { return foliage_point_count_; }
	std::size_t woodyPointCount() const { return woody_point_count_; }
	std::size_t unknownPointCount() const { return unknown_point_count_; }

private:
	VegetationCanopyVoxelIndex index_;
	VegetationMeasuredPoint3d centre_metres_;
	std::size_t foliage_point_count_ = 0u;
	std::size_t woody_point_count_ = 0u;
	std::size_t unknown_point_count_ = 0u;
};
