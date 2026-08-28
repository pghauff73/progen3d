#include "vegetation/service/VegetationPointCloudSourceArtifactValidationService.h"

#include "vegetation/service/VegetationMeasuredCoordinateReferenceTransformService.h"
#include "vegetation/service/VegetationMeasuredSourceArtifactHashService.h"
#include "vegetation/service/VegetationMeasurementUnitNormalizationService.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace {

using Issue = VegetationPointCloudIngestionIssue;
using IssueCode = VegetationPointCloudIngestionIssueCode;

bool is_sha256(const std::string &value)
{
	return value.size() == 64u &&
	       std::all_of(
		       value.begin(), value.end(), [](unsigned char character) {
			       return std::isxdigit(character) != 0;
		       });
}

void require_nonempty(
	const std::string &value,
	IssueCode code,
	const std::string &message,
	std::vector<Issue> &issues)
{
	if (value.empty()) issues.emplace_back(code, std::string(), message);
}

bool supports_schema(
	const std::string &schema,
	const std::vector<std::string> &supported_schemas)
{
	return std::find(supported_schemas.begin(), supported_schemas.end(), schema) !=
	       supported_schemas.end();
}

}

std::vector<VegetationPointCloudIngestionIssue>
VegetationPointCloudSourceArtifactValidationService::validate(
	const VegetationMeasuredSourceArtifact &artifact,
	const VegetationPointCloudIngestionContext &context,
	const std::string &expected_media_type,
	const std::vector<std::string> &supported_source_schema_versions) const
{
	std::vector<Issue> issues;
	if (artifact.schemaVersion() !=
	    "ProGen3D-VegetationMeasuredSourceArtifact-v1") {
		issues.emplace_back(
			IssueCode::UnsupportedArtifactSchema, std::string(),
			"Point-cloud source artifact schema is unsupported.");
	}
	require_nonempty(
		artifact.sourceIdentifier(), IssueCode::EmptySourceIdentifier,
		"Point-cloud source artifact requires a source identifier.", issues);
	require_nonempty(
		artifact.sourceCitation(), IssueCode::EmptySourceCitation,
		"Point-cloud source artifact requires a source citation.", issues);
	require_nonempty(
		artifact.sourceLocator(), IssueCode::EmptySourceLocator,
		"Point-cloud source artifact requires a source locator.", issues);
	if (artifact.mediaType() != expected_media_type) {
		issues.emplace_back(
			IssueCode::UnsupportedMediaType, std::string(),
			"Point-cloud media type does not match the decoder registration.");
	}
	if (!supports_schema(
		    context.sourceSchemaVersion(), supported_source_schema_versions)) {
		issues.emplace_back(
			IssueCode::UnsupportedSourceSchemaVersion, std::string(),
			"Point-cloud source schema version is outside the decoder contract.");
	}
	require_nonempty(
		artifact.coordinateSystem(), IssueCode::EmptyCoordinateSystem,
		"Point-cloud source artifact requires a coordinate system.", issues);
	if (artifact.unitSystem() != "SI" &&
	    artifact.unitSystem() != "DeclaredPerField") {
		issues.emplace_back(
			IssueCode::UnsupportedUnitSystem, std::string(),
			"Point-cloud unit system must be SI or DeclaredPerField.");
	}
	const auto normalized_unit = VegetationMeasurementUnitNormalizationService()
		.normalize(
			1.0, context.sourceCoordinateUnit(),
			VegetationMeasurementQuantityKind::LengthMetres);
	if (!normalized_unit.succeeded()) {
		issues.emplace_back(
			IssueCode::UnsupportedCoordinateUnit, std::string(),
			"Point-cloud coordinate unit is not a supported length unit.");
	}
	require_nonempty(
		artifact.acquisitionMethod(), IssueCode::EmptyAcquisitionMethod,
		"Point-cloud source artifact requires an acquisition method.", issues);
	require_nonempty(
		artifact.uncertaintyStatement(), IssueCode::EmptyUncertaintyStatement,
		"Point-cloud source artifact requires an uncertainty statement.", issues);
	if (context.datasetIdentifier().empty()) {
		issues.emplace_back(
			IssueCode::EmptyDatasetIdentifier, std::string(),
			"Point-cloud ingestion context requires a dataset identifier.");
	}
	if (context.policy().maximumPayloadBytes() == 0u ||
	    context.policy().maximumPointCount() == 0u) {
		issues.emplace_back(
			IssueCode::InvalidIngestionPolicy, std::string(),
			"Point-cloud ingestion policy requires positive payload and point limits.");
	}
	if (artifact.sourcePayload().empty()) {
		issues.emplace_back(
			IssueCode::EmptyPayload, std::string(),
			"Point-cloud source artifact requires a payload.");
	} else if (artifact.sourcePayload().size() >
	           context.policy().maximumPayloadBytes()) {
		issues.emplace_back(
			IssueCode::OversizedPayload, std::string(),
			"Point-cloud payload exceeds the ingestion policy boundary.");
	}
	if (artifact.payloadSha256().empty()) {
		issues.emplace_back(
			IssueCode::MissingSourceHash, std::string(),
			"Point-cloud source artifact requires a payload SHA-256.");
	} else if (!is_sha256(artifact.payloadSha256())) {
		issues.emplace_back(
			IssueCode::InvalidSourceHash, std::string(),
			"Point-cloud payload SHA-256 is malformed.");
	} else if (VegetationMeasuredSourceArtifactHashService()
		           .calculatePayloadSha256(artifact) != artifact.payloadSha256()) {
		issues.emplace_back(
			IssueCode::SourceHashMismatch, std::string(),
			"Point-cloud payload SHA-256 does not match the payload.");
	}
	if (context.coordinateReferenceTransform().has_value()) {
		const auto &transform = *context.coordinateReferenceTransform();
		if (!VegetationMeasuredCoordinateReferenceTransformService()
			     .validate(transform)
			     .valid() ||
		    transform.sourceCoordinateSystem() != artifact.coordinateSystem() ||
		    transform.sourceCoordinateUnit() != context.sourceCoordinateUnit()) {
			issues.emplace_back(
				IssueCode::InvalidCoordinateReferenceTransform, std::string(),
				"Coordinate reference transform is invalid or does not match the source frame and unit.");
		}
	}
	return issues;
}
