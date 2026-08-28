#include "vegetation/service/VegetationMeasuredSourceArtifactValidationService.h"

#include "vegetation/service/VegetationMeasuredSourceArtifactHashService.h"
#include "vegetation/service/VegetationMeasuredCoordinateNormalizationService.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace {

using Issue = VegetationMeasuredSourceAdapterIssue;
using IssueCode = VegetationMeasuredSourceAdapterIssueCode;

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

}

std::vector<VegetationMeasuredSourceAdapterIssue>
VegetationMeasuredSourceArtifactValidationService::validate(
	const VegetationMeasuredSourceArtifact &artifact,
	const std::string &expected_media_type) const
{
	std::vector<Issue> issues;
	if (artifact.schemaVersion() !=
	    "ProGen3D-VegetationMeasuredSourceArtifact-v1") {
		issues.emplace_back(
			IssueCode::UnsupportedArtifactSchema,
			std::string(),
			"Measured source artifact schema is unsupported.");
	}
	require_nonempty(
		artifact.sourceIdentifier(), IssueCode::EmptySourceIdentifier,
		"Measured source artifact requires a source identifier.", issues);
	require_nonempty(
		artifact.sourceCitation(), IssueCode::EmptySourceCitation,
		"Measured source artifact requires a source citation.", issues);
	require_nonempty(
		artifact.sourceLocator(), IssueCode::EmptySourceLocator,
		"Measured source artifact requires a source locator.", issues);
	if (artifact.mediaType() != expected_media_type) {
		issues.emplace_back(
			IssueCode::UnsupportedMediaType,
			std::string(),
			"Measured source artifact media type does not match the adapter.");
	}
	require_nonempty(
		artifact.coordinateSystem(), IssueCode::EmptyCoordinateSystem,
		"Measured source artifact requires a coordinate system.", issues);
	if (!artifact.coordinateSystem().empty() &&
	    !VegetationMeasuredCoordinateNormalizationService()
		     .supportsCoordinateSystem(artifact.coordinateSystem())) {
		issues.emplace_back(
			IssueCode::UnsupportedCoordinateSystem,
			std::string(),
			"Measured source artifact coordinate system is unsupported.");
	}
	if (artifact.unitSystem() != "SI" &&
	    artifact.unitSystem() != "DeclaredPerField") {
		issues.emplace_back(
			IssueCode::UnsupportedUnitSystem,
			std::string(),
			"Measured source artifact unit system must be SI or DeclaredPerField.");
	}
	require_nonempty(
		artifact.acquisitionMethod(), IssueCode::EmptyAcquisitionMethod,
		"Measured source artifact requires an acquisition method.", issues);
	require_nonempty(
		artifact.uncertaintyStatement(), IssueCode::EmptyUncertaintyStatement,
		"Measured source artifact requires an uncertainty statement.", issues);
	if (artifact.sourcePayload().empty()) {
		issues.emplace_back(
			IssueCode::EmptySourcePayload,
			std::string(),
			"Measured source artifact requires a payload.");
	} else if (artifact.sourcePayload().size() > 16u * 1024u * 1024u) {
		issues.emplace_back(
			IssueCode::OversizedSourcePayload,
			std::string(),
			"Measured source artifact exceeds the sixteen-megabyte adapter boundary.");
	}
	if (artifact.payloadSha256().empty()) {
		issues.emplace_back(
			IssueCode::MissingSourceHash,
			std::string(),
			"Measured source artifact requires a payload SHA-256.");
	} else if (!is_sha256(artifact.payloadSha256())) {
		issues.emplace_back(
			IssueCode::InvalidSourceHash,
			std::string(),
			"Measured source artifact payload SHA-256 is malformed.");
	} else if (VegetationMeasuredSourceArtifactHashService()
		           .calculatePayloadSha256(artifact) != artifact.payloadSha256()) {
		issues.emplace_back(
			IssueCode::SourceHashMismatch,
			std::string(),
			"Measured source artifact payload SHA-256 does not match the payload.");
	}
	return issues;
}
