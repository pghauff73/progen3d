#pragma once

#include "vegetation/model/VegetationCalibrationEvidenceBundle.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class VegetationMeasuredSourceAdapterIssueCode
{
	UnsupportedArtifactSchema,
	EmptySourceIdentifier,
	EmptySourceCitation,
	EmptySourceLocator,
	UnsupportedMediaType,
	EmptyCoordinateSystem,
	UnsupportedCoordinateSystem,
	UnsupportedUnitSystem,
	EmptyAcquisitionMethod,
	EmptyUncertaintyStatement,
	EmptySourcePayload,
	OversizedSourcePayload,
	MissingSourceHash,
	InvalidSourceHash,
	SourceHashMismatch,
	MalformedSourcePayload,
	UnsupportedPayloadSchema,
	ArchitectureMismatch,
	MissingRequiredField,
	DuplicateRecordIdentifier,
	InvalidParentReference,
	DisconnectedGraph,
	IncompleteGraph,
	NonFiniteValue,
	InvalidValueRange,
	UnsupportedUnit,
	EmptyObservationSeries,
	DuplicateObservation,
	InvalidObservationOrder
};

class VegetationMeasuredSourceAdapterIssue
{
public:
	VegetationMeasuredSourceAdapterIssue(
		VegetationMeasuredSourceAdapterIssueCode code,
		std::string record_identifier,
		std::string message)
		: code_(code),
		  record_identifier_(std::move(record_identifier)),
		  message_(std::move(message))
	{
	}

	VegetationMeasuredSourceAdapterIssueCode code() const { return code_; }
	const std::string &recordIdentifier() const { return record_identifier_; }
	const std::string &message() const { return message_; }

private:
	VegetationMeasuredSourceAdapterIssueCode code_ =
		VegetationMeasuredSourceAdapterIssueCode::MalformedSourcePayload;
	std::string record_identifier_;
	std::string message_;
};

enum class VegetationMeasuredSourceRecordDisposition
{
	Accepted,
	Deferred,
	Rejected
};

class VegetationMeasuredSourceRecordObservation
{
public:
	VegetationMeasuredSourceRecordObservation(
		std::string record_identifier,
		VegetationMeasuredSourceRecordDisposition disposition,
		std::string explanation)
		: record_identifier_(std::move(record_identifier)),
		  disposition_(disposition),
		  explanation_(std::move(explanation))
	{
	}

	const std::string &recordIdentifier() const { return record_identifier_; }
	VegetationMeasuredSourceRecordDisposition disposition() const
	{
		return disposition_;
	}
	const std::string &explanation() const { return explanation_; }

private:
	std::string record_identifier_;
	VegetationMeasuredSourceRecordDisposition disposition_ =
		VegetationMeasuredSourceRecordDisposition::Deferred;
	std::string explanation_;
};

class VegetationMeasuredSourceAdapterReport
{
public:
	VegetationMeasuredSourceAdapterReport(
		std::vector<VegetationMeasuredSourceAdapterIssue> issues,
		std::vector<VegetationMeasuredSourceRecordObservation> observations,
		std::optional<VegetationCalibrationEvidenceBundle> evidence_bundle)
		: issues_(std::move(issues)),
		  observations_(std::move(observations)),
		  evidence_bundle_(std::move(evidence_bundle))
	{
	}

	bool succeeded() const
	{
		return issues_.empty() && evidence_bundle_.has_value();
	}
	const std::vector<VegetationMeasuredSourceAdapterIssue> &issues() const
	{
		return issues_;
	}
	const std::vector<VegetationMeasuredSourceRecordObservation> &observations()
		const
	{
		return observations_;
	}
	const std::optional<VegetationCalibrationEvidenceBundle> &evidenceBundle()
		const
	{
		return evidence_bundle_;
	}

private:
	std::vector<VegetationMeasuredSourceAdapterIssue> issues_;
	std::vector<VegetationMeasuredSourceRecordObservation> observations_;
	std::optional<VegetationCalibrationEvidenceBundle> evidence_bundle_;
};
