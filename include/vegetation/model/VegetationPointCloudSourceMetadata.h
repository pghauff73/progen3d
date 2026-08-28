#pragma once

#include "vegetation/model/VegetationPointCloudBounds3d.h"
#include "vegetation/model/VegetationPointCloudSourceFormat.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class VegetationPointCloudSourceMetadata
{
public:
	VegetationPointCloudSourceMetadata(
		std::string schema_version,
		std::string source_identifier,
		VegetationPointCloudSourceFormat source_format,
		std::string source_format_version,
		VegetationPointCloudEncoding encoding,
		std::uint64_t declared_point_count,
		std::size_t point_record_length_bytes,
		std::vector<std::string> property_names,
		std::string coordinate_system,
		std::string coordinate_unit,
		std::array<double, 3> coordinate_scales,
		std::array<double, 3> coordinate_offsets,
		std::optional<VegetationPointCloudBounds3d>
			declared_bounds_metres,
		std::string source_payload_sha256)
		: schema_version_(std::move(schema_version)),
		  source_identifier_(std::move(source_identifier)),
		  source_format_(source_format),
		  source_format_version_(std::move(source_format_version)),
		  encoding_(encoding),
		  declared_point_count_(declared_point_count),
		  point_record_length_bytes_(point_record_length_bytes),
		  property_names_(std::move(property_names)),
		  coordinate_system_(std::move(coordinate_system)),
		  coordinate_unit_(std::move(coordinate_unit)),
		  coordinate_scales_(coordinate_scales),
		  coordinate_offsets_(coordinate_offsets),
		  declared_bounds_metres_(std::move(declared_bounds_metres)),
		  source_payload_sha256_(std::move(source_payload_sha256))
	{
	}

	const std::string &schemaVersion() const { return schema_version_; }
	const std::string &sourceIdentifier() const { return source_identifier_; }
	VegetationPointCloudSourceFormat sourceFormat() const
	{
		return source_format_;
	}
	const std::string &sourceFormatVersion() const
	{
		return source_format_version_;
	}
	VegetationPointCloudEncoding encoding() const { return encoding_; }
	std::uint64_t declaredPointCount() const { return declared_point_count_; }
	std::size_t pointRecordLengthBytes() const
	{
		return point_record_length_bytes_;
	}
	const std::vector<std::string> &propertyNames() const
	{
		return property_names_;
	}
	const std::string &coordinateSystem() const { return coordinate_system_; }
	const std::string &coordinateUnit() const { return coordinate_unit_; }
	const std::array<double, 3> &coordinateScales() const
	{
		return coordinate_scales_;
	}
	const std::array<double, 3> &coordinateOffsets() const
	{
		return coordinate_offsets_;
	}
	const std::optional<VegetationPointCloudBounds3d> &
	declaredBoundsMetres() const
	{
		return declared_bounds_metres_;
	}
	const std::string &sourcePayloadSha256() const
	{
		return source_payload_sha256_;
	}

private:
	std::string schema_version_;
	std::string source_identifier_;
	VegetationPointCloudSourceFormat source_format_ =
		VegetationPointCloudSourceFormat::Ply;
	std::string source_format_version_;
	VegetationPointCloudEncoding encoding_ = VegetationPointCloudEncoding::Ascii;
	std::uint64_t declared_point_count_ = 0u;
	std::size_t point_record_length_bytes_ = 0u;
	std::vector<std::string> property_names_;
	std::string coordinate_system_;
	std::string coordinate_unit_;
	std::array<double, 3> coordinate_scales_{{1.0, 1.0, 1.0}};
	std::array<double, 3> coordinate_offsets_{{0.0, 0.0, 0.0}};
	std::optional<VegetationPointCloudBounds3d> declared_bounds_metres_;
	std::string source_payload_sha256_;
};
