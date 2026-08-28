#pragma once

#include <array>
#include <string>
#include <utility>

class VegetationMeasuredCoordinateReferenceTransform
{
public:
	VegetationMeasuredCoordinateReferenceTransform(
		std::string schema_version,
		std::string transform_identifier,
		std::string source_coordinate_system,
		std::string target_coordinate_system,
		std::string source_coordinate_unit,
		std::string target_coordinate_unit,
		std::array<double, 16> source_metres_to_target_metres,
		std::string uncertainty_statement,
		std::string evidence_identifier,
		std::string evidence_sha256)
		: schema_version_(std::move(schema_version)),
		  transform_identifier_(std::move(transform_identifier)),
		  source_coordinate_system_(std::move(source_coordinate_system)),
		  target_coordinate_system_(std::move(target_coordinate_system)),
		  source_coordinate_unit_(std::move(source_coordinate_unit)),
		  target_coordinate_unit_(std::move(target_coordinate_unit)),
		  source_metres_to_target_metres_(source_metres_to_target_metres),
		  uncertainty_statement_(std::move(uncertainty_statement)),
		  evidence_identifier_(std::move(evidence_identifier)),
		  evidence_sha256_(std::move(evidence_sha256))
	{
	}

	const std::string &schemaVersion() const { return schema_version_; }
	const std::string &transformIdentifier() const
	{
		return transform_identifier_;
	}
	const std::string &sourceCoordinateSystem() const
	{
		return source_coordinate_system_;
	}
	const std::string &targetCoordinateSystem() const
	{
		return target_coordinate_system_;
	}
	const std::string &sourceCoordinateUnit() const
	{
		return source_coordinate_unit_;
	}
	const std::string &targetCoordinateUnit() const
	{
		return target_coordinate_unit_;
	}
	const std::array<double, 16> &sourceMetresToTargetMetres() const
	{
		return source_metres_to_target_metres_;
	}
	const std::string &uncertaintyStatement() const
	{
		return uncertainty_statement_;
	}
	const std::string &evidenceIdentifier() const
	{
		return evidence_identifier_;
	}
	const std::string &evidenceSha256() const { return evidence_sha256_; }

private:
	std::string schema_version_;
	std::string transform_identifier_;
	std::string source_coordinate_system_;
	std::string target_coordinate_system_;
	std::string source_coordinate_unit_;
	std::string target_coordinate_unit_;
	std::array<double, 16> source_metres_to_target_metres_{};
	std::string uncertainty_statement_;
	std::string evidence_identifier_;
	std::string evidence_sha256_;
};
