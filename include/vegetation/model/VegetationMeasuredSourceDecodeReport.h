#pragma once

#include "vegetation/model/VegetationMeasuredSourceArtifact.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class VegetationMeasuredSourceDecodeIssueCode
{
	UnsupportedArtifactSchema,
	EmptySourceIdentifier,
	EmptySourceCitation,
	EmptySourceLocator,
	UnsupportedSourceMediaType,
	UnsupportedSourceSchemaVersion,
	AmbiguousSourceSchemaRegistration,
	EmptyCoordinateSystem,
	UnsupportedUnitSystem,
	EmptyAcquisitionMethod,
	EmptyUncertaintyStatement,
	EmptySourcePayload,
	OversizedSourcePayload,
	MissingSourceHash,
	InvalidSourceHash,
	SourceHashMismatch,
	EmptyCanonicalRecordIdentifier,
	InvalidCoordinateReferenceTransform,
	MalformedSourcePayload,
	MissingRequiredField,
	UnexpectedColumnLayout,
	DuplicateRecordIdentifier,
	InvalidParentReference,
	IncompleteSourceTopology,
	NonFiniteValue,
	InvalidValueRange,
	UnsupportedUnit,
	CanonicalArtifactCreationFailed
};

class VegetationMeasuredSourceDecodeIssue
{
public:
	VegetationMeasuredSourceDecodeIssue(
		VegetationMeasuredSourceDecodeIssueCode code,
		std::string record_identifier,
		std::string message)
		: code_(code),
		  record_identifier_(std::move(record_identifier)),
		  message_(std::move(message))
	{
	}

	VegetationMeasuredSourceDecodeIssueCode code() const { return code_; }
	const std::string &recordIdentifier() const { return record_identifier_; }
	const std::string &message() const { return message_; }

private:
	VegetationMeasuredSourceDecodeIssueCode code_ =
		VegetationMeasuredSourceDecodeIssueCode::MalformedSourcePayload;
	std::string record_identifier_;
	std::string message_;
};

enum class VegetationMeasuredSourceDecodeObservationKind
{
	DecodedRecord,
	DerivedGeometry,
	LossyConversion,
	DeferredField,
	RejectedRecord
};

class VegetationMeasuredSourceDecodeObservation
{
public:
	VegetationMeasuredSourceDecodeObservation(
		std::string record_identifier,
		VegetationMeasuredSourceDecodeObservationKind kind,
		std::string explanation)
		: record_identifier_(std::move(record_identifier)),
		  kind_(kind),
		  explanation_(std::move(explanation))
	{
	}

	const std::string &recordIdentifier() const { return record_identifier_; }
	VegetationMeasuredSourceDecodeObservationKind kind() const { return kind_; }
	const std::string &explanation() const { return explanation_; }

private:
	std::string record_identifier_;
	VegetationMeasuredSourceDecodeObservationKind kind_ =
		VegetationMeasuredSourceDecodeObservationKind::DeferredField;
	std::string explanation_;
};

class VegetationMeasuredSourceDecodeReport
{
public:
	VegetationMeasuredSourceDecodeReport(
		std::string decoder_identifier,
		std::string source_identifier,
		std::string source_payload_sha256,
		std::vector<VegetationMeasuredSourceDecodeIssue> issues,
		std::vector<VegetationMeasuredSourceDecodeObservation> observations,
		std::optional<VegetationMeasuredSourceArtifact> canonical_artifact)
		: decoder_identifier_(std::move(decoder_identifier)),
		  source_identifier_(std::move(source_identifier)),
		  source_payload_sha256_(std::move(source_payload_sha256)),
		  issues_(std::move(issues)),
		  observations_(std::move(observations)),
		  canonical_artifact_(std::move(canonical_artifact))
	{
	}

	bool succeeded() const
	{
		return issues_.empty() && canonical_artifact_.has_value();
	}
	const std::string &decoderIdentifier() const { return decoder_identifier_; }
	const std::string &sourceIdentifier() const { return source_identifier_; }
	const std::string &sourcePayloadSha256() const
	{
		return source_payload_sha256_;
	}
	const std::vector<VegetationMeasuredSourceDecodeIssue> &issues() const
	{
		return issues_;
	}
	const std::vector<VegetationMeasuredSourceDecodeObservation> &observations()
		const
	{
		return observations_;
	}
	const std::optional<VegetationMeasuredSourceArtifact> &canonicalArtifact()
		const
	{
		return canonical_artifact_;
	}

private:
	std::string decoder_identifier_;
	std::string source_identifier_;
	std::string source_payload_sha256_;
	std::vector<VegetationMeasuredSourceDecodeIssue> issues_;
	std::vector<VegetationMeasuredSourceDecodeObservation> observations_;
	std::optional<VegetationMeasuredSourceArtifact> canonical_artifact_;
};
