#pragma once

#include "vegetation/model/VegetationPointCloudSourceFormat.h"

#include <string>
#include <utility>

enum class VegetationPointCloudCapabilityLevel
{
	Unsupported,
	MetadataOnly,
	PointRecords
};

class VegetationPointCloudFormatCapability
{
public:
	VegetationPointCloudFormatCapability(
		VegetationPointCloudSourceFormat source_format,
		std::string media_type,
		std::string source_schema_version,
		VegetationPointCloudEncoding encoding,
		VegetationPointCloudCapabilityLevel capability_level,
		std::string required_dependency,
		std::string limitation)
		: source_format_(source_format),
		  media_type_(std::move(media_type)),
		  source_schema_version_(std::move(source_schema_version)),
		  encoding_(encoding),
		  capability_level_(capability_level),
		  required_dependency_(std::move(required_dependency)),
		  limitation_(std::move(limitation))
	{
	}

	VegetationPointCloudSourceFormat sourceFormat() const
	{
		return source_format_;
	}
	const std::string &mediaType() const { return media_type_; }
	const std::string &sourceSchemaVersion() const
	{
		return source_schema_version_;
	}
	VegetationPointCloudEncoding encoding() const { return encoding_; }
	VegetationPointCloudCapabilityLevel capabilityLevel() const
	{
		return capability_level_;
	}
	const std::string &requiredDependency() const
	{
		return required_dependency_;
	}
	const std::string &limitation() const { return limitation_; }

private:
	VegetationPointCloudSourceFormat source_format_ =
		VegetationPointCloudSourceFormat::Ply;
	std::string media_type_;
	std::string source_schema_version_;
	VegetationPointCloudEncoding encoding_ = VegetationPointCloudEncoding::Ascii;
	VegetationPointCloudCapabilityLevel capability_level_ =
		VegetationPointCloudCapabilityLevel::Unsupported;
	std::string required_dependency_;
	std::string limitation_;
};
