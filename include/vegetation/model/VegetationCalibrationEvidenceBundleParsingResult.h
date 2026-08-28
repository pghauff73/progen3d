#pragma once

#include "vegetation/model/VegetationCalibrationEvidenceBundle.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class VegetationCalibrationEvidenceBundleParsingCode
{
	InvalidJson,
	MissingPayload,
	MissingRequiredField,
	UnsupportedPlantArchitecture,
	UnsupportedSubjectScope,
	UnsupportedMeasurementKind,
	UnsupportedMeasurementValueKind,
	InvalidMeasurementValue
};

class VegetationCalibrationEvidenceBundleParsingDiagnostic
{
public:
	VegetationCalibrationEvidenceBundleParsingDiagnostic(
		VegetationCalibrationEvidenceBundleParsingCode code,
		std::string message)
		: code_(code), message_(std::move(message))
	{
	}

	VegetationCalibrationEvidenceBundleParsingCode code() const { return code_; }
	const std::string &message() const { return message_; }

private:
	VegetationCalibrationEvidenceBundleParsingCode code_ =
		VegetationCalibrationEvidenceBundleParsingCode::InvalidJson;
	std::string message_;
};

class VegetationCalibrationEvidenceBundleParsingResult
{
public:
	VegetationCalibrationEvidenceBundleParsingResult(
		std::optional<VegetationCalibrationEvidenceBundle> bundle,
		std::vector<VegetationCalibrationEvidenceBundleParsingDiagnostic>
			diagnostics)
		: bundle_(std::move(bundle)), diagnostics_(std::move(diagnostics))
	{
	}

	bool succeeded() const { return bundle_.has_value() && diagnostics_.empty(); }
	const std::optional<VegetationCalibrationEvidenceBundle> &bundle() const
	{
		return bundle_;
	}
	const std::vector<VegetationCalibrationEvidenceBundleParsingDiagnostic> &
	diagnostics() const
	{
		return diagnostics_;
	}

private:
	std::optional<VegetationCalibrationEvidenceBundle> bundle_;
	std::vector<VegetationCalibrationEvidenceBundleParsingDiagnostic>
		diagnostics_;
};
