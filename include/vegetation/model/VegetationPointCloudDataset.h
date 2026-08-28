#pragma once

#include "vegetation/model/VegetationPointCloudBounds3d.h"
#include "vegetation/model/VegetationPointCloudPointRecord.h"
#include "vegetation/model/VegetationPointCloudSourceMetadata.h"

#include <string>
#include <utility>
#include <vector>

class VegetationPointCloudDataset
{
public:
	VegetationPointCloudDataset(
		std::string dataset_identifier,
		VegetationPointCloudSourceMetadata source_metadata,
		std::vector<VegetationPointCloudPointRecord> points,
		VegetationPointCloudBounds3d observed_bounds)
		: dataset_identifier_(std::move(dataset_identifier)),
		  source_metadata_(std::move(source_metadata)),
		  points_(std::move(points)),
		  observed_bounds_(observed_bounds)
	{
	}

	const std::string &datasetIdentifier() const { return dataset_identifier_; }
	const VegetationPointCloudSourceMetadata &sourceMetadata() const
	{
		return source_metadata_;
	}
	const std::vector<VegetationPointCloudPointRecord> &points() const
	{
		return points_;
	}
	const VegetationPointCloudBounds3d &observedBounds() const
	{
		return observed_bounds_;
	}

private:
	std::string dataset_identifier_;
	VegetationPointCloudSourceMetadata source_metadata_;
	std::vector<VegetationPointCloudPointRecord> points_;
	VegetationPointCloudBounds3d observed_bounds_;
};
