#pragma once

#include "vegetation/model/PlantArchitecture.h"
#include "vegetation/model/VegetationMeasuredCoordinateReferenceTransform.h"

#include <optional>
#include <string>
#include <utility>

class VegetationMeasuredSourceDecodeContext
{
public:
	VegetationMeasuredSourceDecodeContext(
		std::string source_schema_version,
		PlantArchitecture plant_architecture,
		std::string canonical_record_identifier,
		std::optional<VegetationMeasuredCoordinateReferenceTransform>
			coordinate_reference_transform = std::nullopt)
		: source_schema_version_(std::move(source_schema_version)),
		  plant_architecture_(plant_architecture),
		  canonical_record_identifier_(
			  std::move(canonical_record_identifier)),
		  coordinate_reference_transform_(
			  std::move(coordinate_reference_transform))
	{
	}

	const std::string &sourceSchemaVersion() const
	{
		return source_schema_version_;
	}
	PlantArchitecture plantArchitecture() const { return plant_architecture_; }
	const std::string &canonicalRecordIdentifier() const
	{
		return canonical_record_identifier_;
	}
	const std::optional<VegetationMeasuredCoordinateReferenceTransform> &
	coordinateReferenceTransform() const
	{
		return coordinate_reference_transform_;
	}

private:
	std::string source_schema_version_;
	PlantArchitecture plant_architecture_ = PlantArchitecture::Tree;
	std::string canonical_record_identifier_;
	std::optional<VegetationMeasuredCoordinateReferenceTransform>
		coordinate_reference_transform_;
};
