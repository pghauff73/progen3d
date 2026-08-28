#pragma once

#include "vegetation/model/VegetationCanopyOccupancyCell.h"
#include "vegetation/model/VegetationCanopyVoxelIndex.h"
#include "vegetation/model/VegetationPointCloudBounds3d.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class VegetationCanopyOccupancyQualityEvidence
{
public:
	VegetationCanopyOccupancyQualityEvidence(
		std::string reconstruction_identifier,
		std::string dataset_identifier,
		std::string source_payload_sha256,
		VegetationMeasuredPoint3d voxel_origin_metres,
		double voxel_edge_length_metres,
		std::size_t total_point_count,
		std::size_t organ_labeled_point_count,
		std::size_t foliage_point_count,
		std::vector<VegetationCanopyVoxelIndex> training_occupied_cells,
		std::vector<VegetationCanopyVoxelIndex> holdout_foliage_cells,
		std::vector<VegetationCanopyOccupancyCell> reconstructed_cells,
		VegetationPointCloudBounds3d crown_bounds_metres)
		: reconstruction_identifier_(std::move(reconstruction_identifier)),
		  dataset_identifier_(std::move(dataset_identifier)),
		  source_payload_sha256_(std::move(source_payload_sha256)),
		  voxel_origin_metres_(voxel_origin_metres),
		  voxel_edge_length_metres_(voxel_edge_length_metres),
		  total_point_count_(total_point_count),
		  organ_labeled_point_count_(organ_labeled_point_count),
		  foliage_point_count_(foliage_point_count),
		  training_occupied_cells_(std::move(training_occupied_cells)),
		  holdout_foliage_cells_(std::move(holdout_foliage_cells)),
		  reconstructed_cells_(std::move(reconstructed_cells)),
		  crown_bounds_metres_(crown_bounds_metres)
	{
	}

	const std::string &reconstructionIdentifier() const
	{
		return reconstruction_identifier_;
	}
	const std::string &datasetIdentifier() const { return dataset_identifier_; }
	const std::string &sourcePayloadSha256() const
	{
		return source_payload_sha256_;
	}
	const VegetationMeasuredPoint3d &voxelOriginMetres() const
	{
		return voxel_origin_metres_;
	}
	double voxelEdgeLengthMetres() const { return voxel_edge_length_metres_; }
	std::size_t totalPointCount() const { return total_point_count_; }
	std::size_t organLabeledPointCount() const
	{
		return organ_labeled_point_count_;
	}
	std::size_t foliagePointCount() const { return foliage_point_count_; }
	const std::vector<VegetationCanopyVoxelIndex> &trainingOccupiedCells() const
	{
		return training_occupied_cells_;
	}
	const std::vector<VegetationCanopyVoxelIndex> &holdoutFoliageCells() const
	{
		return holdout_foliage_cells_;
	}
	const std::vector<VegetationCanopyOccupancyCell> &reconstructedCells() const
	{
		return reconstructed_cells_;
	}
	const VegetationPointCloudBounds3d &crownBoundsMetres() const
	{
		return crown_bounds_metres_;
	}

private:
	std::string reconstruction_identifier_;
	std::string dataset_identifier_;
	std::string source_payload_sha256_;
	VegetationMeasuredPoint3d voxel_origin_metres_;
	double voxel_edge_length_metres_ = 0.0;
	std::size_t total_point_count_ = 0u;
	std::size_t organ_labeled_point_count_ = 0u;
	std::size_t foliage_point_count_ = 0u;
	std::vector<VegetationCanopyVoxelIndex> training_occupied_cells_;
	std::vector<VegetationCanopyVoxelIndex> holdout_foliage_cells_;
	std::vector<VegetationCanopyOccupancyCell> reconstructed_cells_;
	VegetationPointCloudBounds3d crown_bounds_metres_;
};
