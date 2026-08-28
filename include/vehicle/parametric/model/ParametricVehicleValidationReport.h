#pragma once

#include <string>
#include <utility>
#include <vector>

enum class ParametricVehicleValidationCode
{
	InvalidCoordinateFrame,
	InvalidPackage,
	InvalidWheelParameters,
	InvalidStationFunction,
	InvalidPowertrainIntent,
	InvalidFieldDefinition,
	InvalidGenerationPolicy,
	NonfiniteGeneratedMesh,
	PackageBoundsMismatch,
	NonWatertightGeneratedMesh,
	MissingCharacterCurves,
	MissingBodySections,
	MissingObservations,
	MissingProvenance,
	NondeterministicGeneration,
	SilhouetteThresholdFailure,
	Mvp25CompatibilityFailure
};

class ParametricVehicleValidationIssue
{
public:
	ParametricVehicleValidationIssue(
		ParametricVehicleValidationCode code,
		std::string message,
		std::vector<std::string> object_identifiers = {})
		: code_(code),
		  message_(std::move(message)),
		  object_identifiers_(std::move(object_identifiers))
	{
	}

	ParametricVehicleValidationCode code() const { return code_; }
	const std::string &message() const { return message_; }
	const std::vector<std::string> &objectIdentifiers() const
	{
		return object_identifiers_;
	}

private:
	ParametricVehicleValidationCode code_ =
		ParametricVehicleValidationCode::InvalidPackage;
	std::string message_;
	std::vector<std::string> object_identifiers_;
};

class ParametricVehicleValidationReport
{
public:
	void addIssue(ParametricVehicleValidationIssue issue)
	{
		issues_.push_back(std::move(issue));
	}

	void append(const ParametricVehicleValidationReport &other)
	{
		issues_.insert(issues_.end(), other.issues_.begin(), other.issues_.end());
	}

	bool isValid() const { return issues_.empty(); }
	const std::vector<ParametricVehicleValidationIssue> &issues() const
	{
		return issues_;
	}

private:
	std::vector<ParametricVehicleValidationIssue> issues_;
};

const char *parametricVehicleValidationCodeName(ParametricVehicleValidationCode code);
