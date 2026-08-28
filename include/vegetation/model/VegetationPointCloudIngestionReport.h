#pragma once

#include "vegetation/model/VegetationPointCloudDataset.h"
#include "vegetation/model/VegetationPointCloudSourceMetadata.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class VegetationPointCloudIngestionIssueCode
{
	UnsupportedArtifactSchema,
	EmptySourceIdentifier,
	EmptySourceCitation,
	EmptySourceLocator,
	UnsupportedMediaType,
	UnsupportedSourceSchemaVersion,
	UnsupportedEncoding,
	MissingRequiredDependency,
	EmptyCoordinateSystem,
	UnsupportedUnitSystem,
	UnsupportedCoordinateUnit,
	EmptyAcquisitionMethod,
	EmptyUncertaintyStatement,
	EmptyPayload,
	OversizedPayload,
	MissingSourceHash,
	InvalidSourceHash,
	SourceHashMismatch,
	EmptyDatasetIdentifier,
	InvalidIngestionPolicy,
	InvalidCoordinateReferenceTransform,
	MalformedHeader,
	MissingVertexElement,
	MissingCoordinateProperty,
	UnsupportedPropertyType,
	PointCountLimitExceeded,
	TruncatedPointData,
	NonFiniteCoordinate,
	InvalidDeclaredBounds,
	DeclaredBoundsMismatch,
	DuplicatePoint
};

class VegetationPointCloudIngestionIssue
{
public:
	VegetationPointCloudIngestionIssue(
		VegetationPointCloudIngestionIssueCode code,
		std::string record_identifier,
		std::string message)
		: code_(code),
		  record_identifier_(std::move(record_identifier)),
		  message_(std::move(message))
	{
	}

	VegetationPointCloudIngestionIssueCode code() const { return code_; }
	const std::string &recordIdentifier() const { return record_identifier_; }
	const std::string &message() const { return message_; }

private:
	VegetationPointCloudIngestionIssueCode code_ =
		VegetationPointCloudIngestionIssueCode::MalformedHeader;
	std::string record_identifier_;
	std::string message_;
};

enum class VegetationPointCloudIngestionObservationKind
{
	AcceptedPoint,
	DeferredProperty,
	MetadataOnly,
	RejectedPoint
};

class VegetationPointCloudIngestionObservation
{
public:
	VegetationPointCloudIngestionObservation(
		std::string record_identifier,
		VegetationPointCloudIngestionObservationKind kind,
		std::string explanation)
		: record_identifier_(std::move(record_identifier)),
		  kind_(kind),
		  explanation_(std::move(explanation))
	{
	}

	const std::string &recordIdentifier() const { return record_identifier_; }
	VegetationPointCloudIngestionObservationKind kind() const { return kind_; }
	const std::string &explanation() const { return explanation_; }

private:
	std::string record_identifier_;
	VegetationPointCloudIngestionObservationKind kind_ =
		VegetationPointCloudIngestionObservationKind::MetadataOnly;
	std::string explanation_;
};

class VegetationPointCloudIngestionReport
{
public:
	VegetationPointCloudIngestionReport(
		std::string decoder_identifier,
		std::vector<VegetationPointCloudIngestionIssue> issues,
		std::vector<VegetationPointCloudIngestionObservation> observations,
		std::optional<VegetationPointCloudSourceMetadata> source_metadata,
		std::optional<VegetationPointCloudDataset> dataset)
		: decoder_identifier_(std::move(decoder_identifier)),
		  issues_(std::move(issues)),
		  observations_(std::move(observations)),
		  source_metadata_(std::move(source_metadata)),
		  dataset_(std::move(dataset))
	{
	}

	bool metadataSucceeded() const
	{
		return issues_.empty() && source_metadata_.has_value();
	}
	bool pointsSucceeded() const
	{
		return issues_.empty() && dataset_.has_value();
	}
	const std::string &decoderIdentifier() const { return decoder_identifier_; }
	const std::vector<VegetationPointCloudIngestionIssue> &issues() const
	{
		return issues_;
	}
	const std::vector<VegetationPointCloudIngestionObservation> &observations()
		const
	{
		return observations_;
	}
	const std::optional<VegetationPointCloudSourceMetadata> &sourceMetadata() const
	{
		return source_metadata_;
	}
	const std::optional<VegetationPointCloudDataset> &dataset() const
	{
		return dataset_;
	}

private:
	std::string decoder_identifier_;
	std::vector<VegetationPointCloudIngestionIssue> issues_;
	std::vector<VegetationPointCloudIngestionObservation> observations_;
	std::optional<VegetationPointCloudSourceMetadata> source_metadata_;
	std::optional<VegetationPointCloudDataset> dataset_;
};
