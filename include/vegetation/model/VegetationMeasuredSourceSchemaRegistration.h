#pragma once

#include <string>
#include <utility>

class VegetationMeasuredSourceSchemaRegistration
{
public:
	VegetationMeasuredSourceSchemaRegistration(
		std::string source_media_type,
		std::string source_schema_version,
		std::string decoder_identifier,
		std::string canonical_media_type,
		std::string canonical_schema_version)
		: source_media_type_(std::move(source_media_type)),
		  source_schema_version_(std::move(source_schema_version)),
		  decoder_identifier_(std::move(decoder_identifier)),
		  canonical_media_type_(std::move(canonical_media_type)),
		  canonical_schema_version_(std::move(canonical_schema_version))
	{
	}

	const std::string &sourceMediaType() const { return source_media_type_; }
	const std::string &sourceSchemaVersion() const
	{
		return source_schema_version_;
	}
	const std::string &decoderIdentifier() const { return decoder_identifier_; }
	const std::string &canonicalMediaType() const
	{
		return canonical_media_type_;
	}
	const std::string &canonicalSchemaVersion() const
	{
		return canonical_schema_version_;
	}

private:
	std::string source_media_type_;
	std::string source_schema_version_;
	std::string decoder_identifier_;
	std::string canonical_media_type_;
	std::string canonical_schema_version_;
};
