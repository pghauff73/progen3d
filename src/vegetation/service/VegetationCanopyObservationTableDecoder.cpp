#include "vegetation/service/VegetationCanopyObservationTableDecoder.h"

#include "VegetationDelimitedSourceTableReader.h"
#include "vegetation/model/PlantArchitecture.h"
#include "vegetation/service/VegetationDecodedMeasuredSourceArtifactFactory.h"
#include "vegetation/service/VegetationMeasuredSourceDecoderArtifactValidationService.h"

#include <json/json.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

using Issue = VegetationMeasuredSourceDecodeIssue;
using IssueCode = VegetationMeasuredSourceDecodeIssueCode;
using Observation = VegetationMeasuredSourceDecodeObservation;
using ObservationKind = VegetationMeasuredSourceDecodeObservationKind;

constexpr const char *decoder_identifier =
	"VegetationCanopyObservationTableDecoder";
constexpr const char *source_media_type =
	"text/vnd.progen3d.canopy-observation-table";
constexpr const char *source_schema_version =
	"ProGen3D-CanopyObservationTable-v1";
constexpr const char *canonical_media_type =
	"application/vnd.progen3d.canopy-observation+json";

const std::vector<std::string> expected_header = {
	"observation_identifier",
	"point_cloud_identifier",
	"leaf_area_index",
	"leaf_area_index_unit",
	"crown_gap_fraction",
	"crown_gap_fraction_unit",
	"mean_leaf_inclination",
	"mean_leaf_inclination_unit",
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

Json::Value measurement(double value, const std::string &unit)
{
	Json::Value result(Json::objectValue);
	result["value"] = value;
	result["unit"] = unit;
	return result;
}

}

VegetationMeasuredSourceDecodeReport
VegetationCanopyObservationTableDecoder::decode(
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
			"Canopy observation table header does not match its registered v1 schema.");
	}
	if (table->records().size() != 1u ||
	    (!table->records().empty() && table->records().front().size() != 8u)) {
		issues.emplace_back(
			IssueCode::UnexpectedColumnLayout, std::string(),
			"Canopy observation table requires exactly one eight-field data row.");
	}
	if (!issues.empty()) {
		return report(
			artifact, std::move(issues), std::move(observations), std::nullopt);
	}
	const auto &row = table->records().front();
	const auto leaf_area_index =
		reader.readDecimal(row[2], row[0], "leaf_area_index", issues);
	const auto crown_gap_fraction =
		reader.readDecimal(row[4], row[0], "crown_gap_fraction", issues);
	const auto mean_leaf_inclination =
		reader.readDecimal(row[6], row[0], "mean_leaf_inclination", issues);
	for (std::size_t index : {0u, 1u, 3u, 5u, 7u}) {
		if (row[index].empty()) {
			issues.emplace_back(
				IssueCode::MissingRequiredField, row[0],
				"Canopy observation table contains an empty required text field.");
		}
	}
	if (!leaf_area_index.has_value() || !crown_gap_fraction.has_value() ||
	    !mean_leaf_inclination.has_value() || !issues.empty()) {
		observations.emplace_back(
			row[0], ObservationKind::RejectedRecord,
			"Canopy observation row was rejected while decoding required fields.");
		return report(
			artifact, std::move(issues), std::move(observations), std::nullopt);
	}
	if (row[0] != context.canonicalRecordIdentifier()) {
		observations.emplace_back(
			row[0], ObservationKind::LossyConversion,
			"Source observation identifier was replaced by the explicit canonical record identifier; the raw value remains in source provenance.");
	}
	observations.emplace_back(
		row[0], ObservationKind::DecodedRecord,
		"Canopy optical table row was decoded without changing measurement values or units.");

	Json::Value document(Json::objectValue);
	document["schema_version"] = "ProGen3D-CanopyObservation-v1";
	document["plant_architecture"] =
		plantArchitectureName(context.plantArchitecture());
	document["observation_identifier"] = context.canonicalRecordIdentifier();
	document["point_cloud_identifier"] = row[1];
	document["leaf_area_index"] = measurement(*leaf_area_index, row[3]);
	document["crown_gap_fraction"] =
		measurement(*crown_gap_fraction, row[5]);
	document["mean_leaf_inclination"] =
		measurement(*mean_leaf_inclination, row[7]);
	const VegetationMeasuredSourceArtifact canonical_artifact =
		VegetationDecodedMeasuredSourceArtifactFactory().create(
			artifact, context, decoder_identifier, canonical_media_type,
			std::move(document));
	return report(
		artifact, std::move(issues), std::move(observations), canonical_artifact);
}
