#include "vegetation/service/VegetationPhenologyObservationTableDecoder.h"

#include "VegetationDelimitedSourceTableReader.h"
#include "vegetation/model/PlantArchitecture.h"
#include "vegetation/service/VegetationDecodedMeasuredSourceArtifactFactory.h"
#include "vegetation/service/VegetationMeasuredSourceDecoderArtifactValidationService.h"

#include <json/json.h>

#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

using Issue = VegetationMeasuredSourceDecodeIssue;
using IssueCode = VegetationMeasuredSourceDecodeIssueCode;
using Observation = VegetationMeasuredSourceDecodeObservation;
using ObservationKind = VegetationMeasuredSourceDecodeObservationKind;

constexpr const char *decoder_identifier =
	"VegetationPhenologyObservationTableDecoder";
constexpr const char *source_media_type =
	"text/vnd.progen3d.phenology-series-table";
constexpr const char *source_schema_version =
	"ProGen3D-PhenologyObservationTable-v1";
constexpr const char *canonical_media_type =
	"application/vnd.progen3d.phenology-series+json";

const std::vector<std::string> expected_header = {
	"series_identifier",
	"date",
	"development_stage_identifier",
};

VegetationMeasuredSourceDecodeReport report(
	const VegetationMeasuredSourceArtifact &artifact,
	std::vector<Issue> issues,
	std::vector<Observation> observations,
	std::optional<VegetationMeasuredSourceArtifact> canonical_artifact)
{
	return VegetationMeasuredSourceDecodeReport(
		decoder_identifier,
		artifact.sourceIdentifier(),
		artifact.payloadSha256(),
		std::move(issues),
		std::move(observations),
		std::move(canonical_artifact));
}

}

VegetationMeasuredSourceDecodeReport
VegetationPhenologyObservationTableDecoder::decode(
	const VegetationMeasuredSourceArtifact &artifact,
	const VegetationMeasuredSourceDecodeContext &context) const
{
	std::vector<Issue> issues =
		VegetationMeasuredSourceDecoderArtifactValidationService().validate(
			artifact, context, source_media_type, source_schema_version);
	std::vector<Observation> observations;
	if (!issues.empty()) {
		return report(
			artifact, std::move(issues), std::move(observations), std::nullopt);
	}
	VegetationDelimitedSourceTableReader reader;
	const auto table = reader.read(artifact.sourcePayload(), ',', issues);
	if (!table.has_value()) {
		return report(
			artifact, std::move(issues), std::move(observations), std::nullopt);
	}
	if (table->columnNames() != expected_header) {
		issues.emplace_back(
			IssueCode::UnexpectedColumnLayout, "header",
			"Phenology table header does not match its registered v1 schema.");
	}
	if (table->records().size() < 2u) {
		issues.emplace_back(
			IssueCode::IncompleteSourceTopology, std::string(),
			"Phenology table requires at least two dated observations.");
	}
	Json::Value observation_array(Json::arrayValue);
	std::set<std::string> dates;
	std::string source_series_identifier;
	for (std::size_t index = 0u; index < table->records().size(); ++index) {
		const auto &row = table->records()[index];
		const std::string record_identifier =
			"phenology-row-" + std::to_string(index + 1u);
		if (row.size() != 3u || row[0].empty() || row[1].empty() ||
		    row[2].empty()) {
			issues.emplace_back(
				IssueCode::MissingRequiredField, record_identifier,
				"Phenology row requires series identifier, date, and development stage identifier.");
			observations.emplace_back(
				record_identifier, ObservationKind::RejectedRecord,
				"Phenology row was rejected because a required field is absent.");
			continue;
		}
		if (source_series_identifier.empty()) source_series_identifier = row[0];
		if (row[0] != source_series_identifier) {
			issues.emplace_back(
				IssueCode::DuplicateRecordIdentifier, record_identifier,
				"All phenology rows must belong to one source series identifier.");
		}
		if (!dates.insert(row[1]).second) {
			issues.emplace_back(
				IssueCode::DuplicateRecordIdentifier, record_identifier,
				"Phenology table contains a duplicate observation date.");
		}
		Json::Value observation(Json::objectValue);
		observation["date"] = row[1];
		observation["development_stage_identifier"] = row[2];
		observation_array.append(std::move(observation));
		observations.emplace_back(
			record_identifier, ObservationKind::DecodedRecord,
			"Phenology row was decoded without changing its date or stage identifier.");
	}
	if (!source_series_identifier.empty() &&
	    source_series_identifier != context.canonicalRecordIdentifier()) {
		observations.emplace_back(
			source_series_identifier, ObservationKind::LossyConversion,
			"Source series identifier was replaced by the explicit canonical record identifier; the raw value remains in source provenance.");
	}
	if (!issues.empty()) {
		return report(
			artifact, std::move(issues), std::move(observations), std::nullopt);
	}
	Json::Value document(Json::objectValue);
	document["schema_version"] =
		"ProGen3D-PhenologyObservationSeries-v1";
	document["plant_architecture"] =
		plantArchitectureName(context.plantArchitecture());
	document["series_identifier"] = context.canonicalRecordIdentifier();
	document["observations"] = std::move(observation_array);
	const VegetationMeasuredSourceArtifact canonical_artifact =
		VegetationDecodedMeasuredSourceArtifactFactory().create(
			artifact, context, decoder_identifier, canonical_media_type,
			std::move(document));
	return report(
		artifact, std::move(issues), std::move(observations), canonical_artifact);
}
