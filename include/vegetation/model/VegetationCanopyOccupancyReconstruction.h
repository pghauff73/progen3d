#pragma once

#include "vegetation/model/VegetationCanopyOccupancyCell.h"
#include "vegetation/model/VegetationCanopyOccupancyQualityReport.h"
#include "vegetation/model/VegetationPointCloudBounds3d.h"

#include <string>
#include <utility>
#include <vector>

class VegetationCanopyOccupancyReconstruction
{
public:
	VegetationCanopyOccupancyReconstruction(
		std::string schema_version,
		std::string reconstruction_identifier,
		std::string reconstruction_job_identifier,
		std::string dataset_identifier,
		std::string source_payload_sha256,
		VegetationMeasuredPoint3d voxel_origin_metres,
		double voxel_edge_length_metres,
		std::vector<VegetationCanopyOccupancyCell> cells,
		VegetationPointCloudBounds3d crown_bounds_metres,
		VegetationCanopyOccupancyQualityReport quality_report)
		: schema_version_(std::move(schema_version)),
		  reconstruction_identifier_(std::move(reconstruction_identifier)),
		  reconstruction_job_identifier_(
			  std::move(reconstruction_job_identifier)),
		  dataset_identifier_(std::move(dataset_identifier)),
		  source_payload_sha256_(std::move(source_payload_sha256)),
		  voxel_origin_metres_(voxel_origin_metres),
		  voxel_edge_length_metres_(voxel_edge_length_metres),
		  cells_(std::move(cells)),
		  crown_bounds_metres_(crown_bounds_metres),
		  quality_report_(std::move(quality_report))
	{
	}

	const std::string &schemaVersion() const { return schema_version_; }
	const std::string &reconstructionIdentifier() const
	{
		return reconstruction_identifier_;
	}
	const std::string &reconstructionJobIdentifier() const
	{
		return reconstruction_job_identifier_;
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
	const std::vector<VegetationCanopyOccupancyCell> &cells() const
	{
		return cells_;
	}
	const VegetationPointCloudBounds3d &crownBoundsMetres() const
	{
		return crown_bounds_metres_;
	}
	const VegetationCanopyOccupancyQualityReport &qualityReport() const
	{
		return quality_report_;
	}

private:
	std::string schema_version_;
	std::string reconstruction_identifier_;
	std::string reconstruction_job_identifier_;
	std::string dataset_identifier_;
	std::string source_payload_sha256_;
	VegetationMeasuredPoint3d voxel_origin_metres_;
	double voxel_edge_length_metres_ = 0.0;
	std::vector<VegetationCanopyOccupancyCell> cells_;
	VegetationPointCloudBounds3d crown_bounds_metres_;
	VegetationCanopyOccupancyQualityReport quality_report_;
};
