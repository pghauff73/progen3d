#pragma once

#include <cstddef>
#include <tuple>

class VegetationCanopyVoxelIndex
{
public:
	VegetationCanopyVoxelIndex(
		std::size_t x_index,
		std::size_t y_index,
		std::size_t z_index)
		: x_index_(x_index), y_index_(y_index), z_index_(z_index)
	{
	}

	std::size_t xIndex() const { return x_index_; }
	std::size_t yIndex() const { return y_index_; }
	std::size_t zIndex() const { return z_index_; }

	bool operator<(const VegetationCanopyVoxelIndex &other) const
	{
		return std::tie(x_index_, y_index_, z_index_) <
		       std::tie(other.x_index_, other.y_index_, other.z_index_);
	}

	bool operator==(const VegetationCanopyVoxelIndex &other) const
	{
		return x_index_ == other.x_index_ && y_index_ == other.y_index_ &&
		       z_index_ == other.z_index_;
	}

private:
	std::size_t x_index_ = 0u;
	std::size_t y_index_ = 0u;
	std::size_t z_index_ = 0u;
};
