#pragma once

#include "vegetation/model/VegetationMeasuredCoordinateReferenceTransform.h"
#include "vegetation/model/VegetationPointCloudIngestionPolicy.h"

#include <optional>
#include <string>
#include <utility>

class VegetationPointCloudIngestionContext
{
public:
	VegetationPointCloudIngestionContext(
		std::string source_schema_version,
		std::string dataset_identifier,
		std::string source_coordinate_unit,
		VegetationPointCloudIngestionPolicy policy,
		std::optional<VegetationMeasuredCoordinateReferenceTransform>
			coordinate_reference_transform = std::nullopt)
		: source_schema_version_(std::move(source_schema_version)),
		  dataset_identifier_(std::move(dataset_identifier)),
		  source_coordinate_unit_(std::move(source_coordinate_unit)),
		  policy_(policy),
		  coordinate_reference_transform_(
			  std::move(coordinate_reference_transform))
	{
	}

	const std::string &sourceSchemaVersion() const
	{
		return source_schema_version_;
	}
	const std::string &datasetIdentifier() const { return dataset_identifier_; }
	const std::string &sourceCoordinateUnit() const
	{
		return source_coordinate_unit_;
	}
	const VegetationPointCloudIngestionPolicy &policy() const { return policy_; }
	const std::optional<VegetationMeasuredCoordinateReferenceTransform> &
	coordinateReferenceTransform() const
	{
		return coordinate_reference_transform_;
	}

private:
	std::string source_schema_version_;
	std::string dataset_identifier_;
	std::string source_coordinate_unit_;
	VegetationPointCloudIngestionPolicy policy_;
	std::optional<VegetationMeasuredCoordinateReferenceTransform>
		coordinate_reference_transform_;
};
