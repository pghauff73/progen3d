#pragma once

#include <string>
#include <utility>
#include <vector>

enum class VegetationCalibrationEvidenceBundleValidationCode
{
	UnsupportedSchemaVersion,
	EmptyBundleIdentifier,
	InvalidSubjectScope,
	ArchitectureMismatch,
	ContradictoryIdentity,
	EmptyCoordinateSystem,
	UnsupportedUnitSystem,
	EmptyAcquisitionMethod,
	EmptyUncertaintyStatement,
	MissingSourceReference,
	DuplicateSourceReference,
	MissingEvidenceHash,
	InvalidEvidenceHash,
	PayloadHashMismatch,
	UnknownMeasurementDomain,
	MeasurementDomainMismatch,
	DuplicateMeasurement,
	UnsupportedMeasurementUnit,
	MeasurementValueKindMismatch,
	NonFiniteMeasurementValue,
	InvalidMeasurementValueRange,
	EmptyObservationScope,
	EmptyMeasurementUncertainty,
	MissingMeasurementEvidenceReference,
	UnknownMeasurementEvidenceReference,
	ContradictoryIdentityMeasurement
};

class VegetationCalibrationEvidenceBundleValidationIssue
{
public:
	VegetationCalibrationEvidenceBundleValidationIssue(
		VegetationCalibrationEvidenceBundleValidationCode code,
		std::string message)
		: code_(code), message_(std::move(message))
	{
	}

	VegetationCalibrationEvidenceBundleValidationCode code() const
	{
		return code_;
	}
	const std::string &message() const { return message_; }

private:
	VegetationCalibrationEvidenceBundleValidationCode code_ =
		VegetationCalibrationEvidenceBundleValidationCode::UnsupportedSchemaVersion;
	std::string message_;
};

class VegetationCalibrationEvidenceBundleValidationReport
{
public:
	void addIssue(
		VegetationCalibrationEvidenceBundleValidationCode code,
		std::string message)
	{
		issues_.emplace_back(code, std::move(message));
	}

	bool passed() const { return issues_.empty(); }
	const std::vector<VegetationCalibrationEvidenceBundleValidationIssue> &issues()
		const
	{
		return issues_;
	}

private:
	std::vector<VegetationCalibrationEvidenceBundleValidationIssue> issues_;
};
