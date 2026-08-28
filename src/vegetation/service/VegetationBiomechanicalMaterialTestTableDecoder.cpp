#include "vegetation/service/VegetationBiomechanicalMaterialTestTableDecoder.h"

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
	"VegetationBiomechanicalMaterialTestTableDecoder";
constexpr const char *source_media_type =
	"text/vnd.progen3d.biomechanical-test-table";
constexpr const char *source_schema_version =
	"ProGen3D-BiomechanicalMaterialTestTable-v1";
constexpr const char *canonical_media_type =
	"application/vnd.progen3d.biomechanical-test+json";

const std::vector<std::string> expected_header = {
	"test_identifier",
	"mass_density",
	"mass_density_unit",
	"elastic_modulus",
	"elastic_modulus_unit",
	"damping_ratio",
	"damping_ratio_unit",
	"drag_coefficient",
	"drag_coefficient_unit",
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
VegetationBiomechanicalMaterialTestTableDecoder::decode(
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
			"Biomechanical table header does not match its registered v1 schema.");
	}
	if (table->records().size() != 1u ||
	    (!table->records().empty() && table->records().front().size() != 9u)) {
		issues.emplace_back(
			IssueCode::UnexpectedColumnLayout, std::string(),
			"Biomechanical table requires exactly one nine-field data row.");
	}
	if (!issues.empty()) {
		return report(
			artifact, std::move(issues), std::move(observations), std::nullopt);
	}
	const auto &row = table->records().front();
	const auto density =
		reader.readDecimal(row[1], row[0], "mass_density", issues);
	const auto modulus =
		reader.readDecimal(row[3], row[0], "elastic_modulus", issues);
	const auto damping =
		reader.readDecimal(row[5], row[0], "damping_ratio", issues);
	const auto drag =
		reader.readDecimal(row[7], row[0], "drag_coefficient", issues);
	for (std::size_t index : {0u, 2u, 4u, 6u, 8u}) {
		if (row[index].empty()) {
			issues.emplace_back(
				IssueCode::MissingRequiredField, row[0],
				"Biomechanical table contains an empty required text field.");
		}
	}
	if (!density.has_value() || !modulus.has_value() || !damping.has_value() ||
	    !drag.has_value() || !issues.empty()) {
		observations.emplace_back(
			row[0], ObservationKind::RejectedRecord,
			"Biomechanical test row was rejected while decoding required fields.");
		return report(
			artifact, std::move(issues), std::move(observations), std::nullopt);
	}
	if (row[0] != context.canonicalRecordIdentifier()) {
		observations.emplace_back(
			row[0], ObservationKind::LossyConversion,
			"Source test identifier was replaced by the explicit canonical record identifier; the raw value remains in source provenance.");
	}
	observations.emplace_back(
		row[0], ObservationKind::DecodedRecord,
		"Biomechanical test table row was decoded without changing measurement values or units.");
	Json::Value document(Json::objectValue);
	document["schema_version"] =
		"ProGen3D-BiomechanicalMaterialTest-v1";
	document["plant_architecture"] =
		plantArchitectureName(context.plantArchitecture());
	document["test_identifier"] = context.canonicalRecordIdentifier();
	document["mass_density"] = measurement(*density, row[2]);
	document["elastic_modulus"] = measurement(*modulus, row[4]);
	document["damping_ratio"] = measurement(*damping, row[6]);
	document["drag_coefficient"] = measurement(*drag, row[8]);
	const VegetationMeasuredSourceArtifact canonical_artifact =
		VegetationDecodedMeasuredSourceArtifactFactory().create(
			artifact, context, decoder_identifier, canonical_media_type,
			std::move(document));
	return report(
		artifact, std::move(issues), std::move(observations), canonical_artifact);
}
