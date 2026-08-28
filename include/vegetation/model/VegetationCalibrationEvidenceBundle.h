#pragma once

#include "vegetation/model/PlantArchitecture.h"
#include "vegetation/model/VegetationCalibrationMeasurement.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class VegetationCalibrationSubjectScopeKind
{
	ArchitecturalArchetype,
	BotanicalTaxon,
	Cultivar,
	MeasuredSpecimen
};

const char *vegetationCalibrationSubjectScopeKindName(
	VegetationCalibrationSubjectScopeKind kind);
std::optional<VegetationCalibrationSubjectScopeKind>
vegetationCalibrationSubjectScopeKindFromName(const std::string &name);

class VegetationCalibrationSubjectScope
{
public:
	VegetationCalibrationSubjectScope(
		VegetationCalibrationSubjectScopeKind kind,
		PlantArchitecture architecture,
		std::string subject_identifier,
		std::string scientific_name = std::string(),
		std::string cultivar_name = std::string(),
		std::string specimen_identifier = std::string())
		: kind_(kind),
		  architecture_(architecture),
		  subject_identifier_(std::move(subject_identifier)),
		  scientific_name_(std::move(scientific_name)),
		  cultivar_name_(std::move(cultivar_name)),
		  specimen_identifier_(std::move(specimen_identifier))
	{
	}

	VegetationCalibrationSubjectScopeKind kind() const { return kind_; }
	PlantArchitecture architecture() const { return architecture_; }
	const std::string &subjectIdentifier() const { return subject_identifier_; }
	const std::string &scientificName() const { return scientific_name_; }
	const std::string &cultivarName() const { return cultivar_name_; }
	const std::string &specimenIdentifier() const
	{
		return specimen_identifier_;
	}

private:
	VegetationCalibrationSubjectScopeKind kind_ =
		VegetationCalibrationSubjectScopeKind::ArchitecturalArchetype;
	PlantArchitecture architecture_ = PlantArchitecture::Tree;
	std::string subject_identifier_;
	std::string scientific_name_;
	std::string cultivar_name_;
	std::string specimen_identifier_;
};

class VegetationCalibrationSourceReference
{
public:
	VegetationCalibrationSourceReference(
		std::string source_identifier,
		std::string citation,
		std::string locator,
		std::string sha256)
		: source_identifier_(std::move(source_identifier)),
		  citation_(std::move(citation)),
		  locator_(std::move(locator)),
		  sha256_(std::move(sha256))
	{
	}

	const std::string &sourceIdentifier() const { return source_identifier_; }
	const std::string &citation() const { return citation_; }
	const std::string &locator() const { return locator_; }
	const std::string &sha256() const { return sha256_; }

private:
	std::string source_identifier_;
	std::string citation_;
	std::string locator_;
	std::string sha256_;
};

class VegetationCalibrationEvidenceBundle
{
public:
	VegetationCalibrationEvidenceBundle(
		std::string schema_version,
		std::string bundle_identifier,
		VegetationCalibrationSubjectScope subject_scope,
		std::string coordinate_system,
		std::string unit_system,
		std::string acquisition_method,
		std::string uncertainty_statement,
		std::vector<VegetationCalibrationSourceReference> source_references,
		std::vector<VegetationCalibrationMeasurement> measurements,
		std::string payload_sha256)
		: schema_version_(std::move(schema_version)),
		  bundle_identifier_(std::move(bundle_identifier)),
		  subject_scope_(std::move(subject_scope)),
		  coordinate_system_(std::move(coordinate_system)),
		  unit_system_(std::move(unit_system)),
		  acquisition_method_(std::move(acquisition_method)),
		  uncertainty_statement_(std::move(uncertainty_statement)),
		  source_references_(std::move(source_references)),
		  measurements_(std::move(measurements)),
		  payload_sha256_(std::move(payload_sha256))
	{
	}

	const std::string &schemaVersion() const { return schema_version_; }
	const std::string &bundleIdentifier() const { return bundle_identifier_; }
	const VegetationCalibrationSubjectScope &subjectScope() const
	{
		return subject_scope_;
	}
	const std::string &coordinateSystem() const { return coordinate_system_; }
	const std::string &unitSystem() const { return unit_system_; }
	const std::string &acquisitionMethod() const
	{
		return acquisition_method_;
	}
	const std::string &uncertaintyStatement() const
	{
		return uncertainty_statement_;
	}
	const std::vector<VegetationCalibrationSourceReference> &sourceReferences()
		const
	{
		return source_references_;
	}
	const std::vector<VegetationCalibrationMeasurement> &measurements() const
	{
		return measurements_;
	}
	const std::string &payloadSha256() const { return payload_sha256_; }

	VegetationCalibrationEvidenceBundle withPayloadSha256(
		std::string payload_sha256) const
	{
		return VegetationCalibrationEvidenceBundle(
			schema_version_,
			bundle_identifier_,
			subject_scope_,
			coordinate_system_,
			unit_system_,
			acquisition_method_,
			uncertainty_statement_,
			source_references_,
			measurements_,
			std::move(payload_sha256));
	}

private:
	std::string schema_version_;
	std::string bundle_identifier_;
	VegetationCalibrationSubjectScope subject_scope_;
	std::string coordinate_system_;
	std::string unit_system_;
	std::string acquisition_method_;
	std::string uncertainty_statement_;
	std::vector<VegetationCalibrationSourceReference> source_references_;
	std::vector<VegetationCalibrationMeasurement> measurements_;
	std::string payload_sha256_;
};
