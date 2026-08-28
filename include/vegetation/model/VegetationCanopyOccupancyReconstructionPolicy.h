#pragma once

#include <cstddef>

class VegetationCanopyOccupancyReconstructionPolicy
{
public:
	VegetationCanopyOccupancyReconstructionPolicy(
		double voxel_edge_length_metres,
		std::size_t minimum_foliage_points_per_cell,
		std::size_t maximum_occupied_cell_count,
		std::size_t holdout_divisor,
		std::size_t holdout_neighbor_radius_cells,
		double minimum_holdout_recall,
		double minimum_organ_label_fraction)
		: voxel_edge_length_metres_(voxel_edge_length_metres),
		  minimum_foliage_points_per_cell_(minimum_foliage_points_per_cell),
		  maximum_occupied_cell_count_(maximum_occupied_cell_count),
		  holdout_divisor_(holdout_divisor),
		  holdout_neighbor_radius_cells_(holdout_neighbor_radius_cells),
		  minimum_holdout_recall_(minimum_holdout_recall),
		  minimum_organ_label_fraction_(minimum_organ_label_fraction)
	{
	}

	double voxelEdgeLengthMetres() const { return voxel_edge_length_metres_; }
	std::size_t minimumFoliagePointsPerCell() const
	{
		return minimum_foliage_points_per_cell_;
	}
	std::size_t maximumOccupiedCellCount() const
	{
		return maximum_occupied_cell_count_;
	}
	std::size_t holdoutDivisor() const { return holdout_divisor_; }
	std::size_t holdoutNeighborRadiusCells() const
	{
		return holdout_neighbor_radius_cells_;
	}
	double minimumHoldoutRecall() const { return minimum_holdout_recall_; }
	double minimumOrganLabelFraction() const
	{
		return minimum_organ_label_fraction_;
	}

private:
	double voxel_edge_length_metres_ = 0.0;
	std::size_t minimum_foliage_points_per_cell_ = 0u;
	std::size_t maximum_occupied_cell_count_ = 0u;
	std::size_t holdout_divisor_ = 0u;
	std::size_t holdout_neighbor_radius_cells_ = 0u;
	double minimum_holdout_recall_ = 0.0;
	double minimum_organ_label_fraction_ = 0.0;
};
