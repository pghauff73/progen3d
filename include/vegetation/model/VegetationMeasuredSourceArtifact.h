#pragma once

#include <string>
#include <utility>

class VegetationMeasuredSourceArtifact
{
public:
	VegetationMeasuredSourceArtifact(
		std::string schema_version,
		std::string source_identifier,
		std::string source_citation,
		std::string source_locator,
		std::string media_type,
		std::string coordinate_system,
		std::string unit_system,
		std::string acquisition_method,
		std::string uncertainty_statement,
		std::string source_payload,
		std::string payload_sha256)
		: schema_version_(std::move(schema_version)),
		  source_identifier_(std::move(source_identifier)),
		  source_citation_(std::move(source_citation)),
		  source_locator_(std::move(source_locator)),
		  media_type_(std::move(media_type)),
		  coordinate_system_(std::move(coordinate_system)),
		  unit_system_(std::move(unit_system)),
		  acquisition_method_(std::move(acquisition_method)),
		  uncertainty_statement_(std::move(uncertainty_statement)),
		  source_payload_(std::move(source_payload)),
		  payload_sha256_(std::move(payload_sha256))
	{
	}

	const std::string &schemaVersion() const { return schema_version_; }
	const std::string &sourceIdentifier() const { return source_identifier_; }
	const std::string &sourceCitation() const { return source_citation_; }
	const std::string &sourceLocator() const { return source_locator_; }
	const std::string &mediaType() const { return media_type_; }
	const std::string &coordinateSystem() const { return coordinate_system_; }
	const std::string &unitSystem() const { return unit_system_; }
	const std::string &acquisitionMethod() const { return acquisition_method_; }
	const std::string &uncertaintyStatement() const
	{
		return uncertainty_statement_;
	}
	const std::string &sourcePayload() const { return source_payload_; }
	const std::string &payloadSha256() const { return payload_sha256_; }

	VegetationMeasuredSourceArtifact withPayloadSha256(
		std::string payload_sha256) const;

private:
	std::string schema_version_;
	std::string source_identifier_;
	std::string source_citation_;
	std::string source_locator_;
	std::string media_type_;
	std::string coordinate_system_;
	std::string unit_system_;
	std::string acquisition_method_;
	std::string uncertainty_statement_;
	std::string source_payload_;
	std::string payload_sha256_;
};
