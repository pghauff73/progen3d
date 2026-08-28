#pragma once

#include "vegetation/model/VegetationMeasuredPoint3d.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class VegetationWoodyCoverSet
{
public:
	VegetationWoodyCoverSet(
		std::string cover_identifier,
		std::int64_t grid_x,
		std::int64_t grid_y,
		std::int64_t grid_z,
		VegetationMeasuredPoint3d centroid_metres,
		std::vector<std::size_t> source_point_indices)
		: cover_identifier_(std::move(cover_identifier)),
		  grid_x_(grid_x),
		  grid_y_(grid_y),
		  grid_z_(grid_z),
		  centroid_metres_(centroid_metres),
		  source_point_indices_(std::move(source_point_indices))
	{
	}

	const std::string &coverIdentifier() const { return cover_identifier_; }
	std::int64_t gridX() const { return grid_x_; }
	std::int64_t gridY() const { return grid_y_; }
	std::int64_t gridZ() const { return grid_z_; }
	const VegetationMeasuredPoint3d &centroidMetres() const
	{
		return centroid_metres_;
	}
	const std::vector<std::size_t> &sourcePointIndices() const
	{
		return source_point_indices_;
	}

private:
	std::string cover_identifier_;
	std::int64_t grid_x_ = 0;
	std::int64_t grid_y_ = 0;
	std::int64_t grid_z_ = 0;
	VegetationMeasuredPoint3d centroid_metres_{0.0, 0.0, 0.0};
	std::vector<std::size_t> source_point_indices_;
};
