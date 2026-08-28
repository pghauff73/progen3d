#include "vegetation/service/VegetationCalibrationEvidenceBundleSerializationService.h"

#include <json/json.h>

#include <algorithm>
#include <string>
#include <vector>

namespace {

Json::Value string_array(std::vector<std::string> values)
{
	std::sort(values.begin(), values.end());
	Json::Value array(Json::arrayValue);
	for (const std::string &value : values) array.append(value);
	return array;
}

Json::Value measurement_value(
	const VegetationCalibrationMeasurementValue &value)
{
	switch (value.kind()) {
	case VegetationCalibrationMeasurementValueKind::Decimal:
		return Json::Value(value.decimalValue().value());
	case VegetationCalibrationMeasurementValueKind::Count:
		return Json::Value(
			static_cast<Json::UInt64>(value.countValue().value()));
	case VegetationCalibrationMeasurementValueKind::Text:
		return Json::Value(*value.textValue());
	case VegetationCalibrationMeasurementValueKind::IdentifierList:
		return string_array(*value.identifierListValue());
	}
	return Json::Value();
}

Json::Value measurement_json(
	const VegetationCalibrationMeasurement &measurement)
{
	Json::Value json(Json::objectValue);
	json["kind"] = vegetationCalibrationMeasurementKindName(measurement.kind());
	json["domain"] = measurement.declaredDomainName();
	json["value_kind"] = vegetationCalibrationMeasurementValueKindName(
		measurement.value().kind());
	json["value"] = measurement_value(measurement.value());
	json["unit"] = measurement.unit();
	json["observation_scope"] = measurement.observationScope();
	json["uncertainty_statement"] = measurement.uncertaintyStatement();
	json["evidence_source_identifiers"] =
		string_array(measurement.evidenceSourceIdentifiers());
	return json;
}

Json::Value payload_json(const VegetationCalibrationEvidenceBundle &bundle)
{
	Json::Value payload(Json::objectValue);
	payload["schema_version"] = bundle.schemaVersion();
	payload["bundle_identifier"] = bundle.bundleIdentifier();

	const VegetationCalibrationSubjectScope &scope = bundle.subjectScope();
	Json::Value subject_scope(Json::objectValue);
	subject_scope["kind"] = vegetationCalibrationSubjectScopeKindName(scope.kind());
	subject_scope["plant_architecture"] =
		plantArchitectureName(scope.architecture());
	subject_scope["subject_identifier"] = scope.subjectIdentifier();
	subject_scope["scientific_name"] = scope.scientificName();
	subject_scope["cultivar_name"] = scope.cultivarName();
	subject_scope["specimen_identifier"] = scope.specimenIdentifier();
	payload["subject_scope"] = subject_scope;

	payload["coordinate_system"] = bundle.coordinateSystem();
	payload["unit_system"] = bundle.unitSystem();
	payload["acquisition_method"] = bundle.acquisitionMethod();
	payload["uncertainty_statement"] = bundle.uncertaintyStatement();

	std::vector<VegetationCalibrationSourceReference> source_references =
		bundle.sourceReferences();
	std::sort(
		source_references.begin(), source_references.end(),
		[](const VegetationCalibrationSourceReference &left,
		   const VegetationCalibrationSourceReference &right) {
			return left.sourceIdentifier() < right.sourceIdentifier();
		});
	Json::Value sources(Json::arrayValue);
	for (const VegetationCalibrationSourceReference &source : source_references) {
		Json::Value source_json(Json::objectValue);
		source_json["source_identifier"] = source.sourceIdentifier();
		source_json["citation"] = source.citation();
		source_json["locator"] = source.locator();
		source_json["sha256"] = source.sha256();
		sources.append(source_json);
	}
	payload["source_references"] = sources;

	std::vector<VegetationCalibrationMeasurement> measurements =
		bundle.measurements();
	std::sort(
		measurements.begin(), measurements.end(),
		[](const VegetationCalibrationMeasurement &left,
		   const VegetationCalibrationMeasurement &right) {
			return std::string(vegetationCalibrationMeasurementKindName(left.kind())) <
			       vegetationCalibrationMeasurementKindName(right.kind());
		});
	Json::Value measurement_array(Json::arrayValue);
	for (const VegetationCalibrationMeasurement &measurement : measurements) {
		measurement_array.append(measurement_json(measurement));
	}
	payload["measurements"] = measurement_array;
	return payload;
}

std::string compact_json(const Json::Value &json)
{
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "";
	writer["commentStyle"] = "None";
	writer["emitUTF8"] = true;
	return Json::writeString(writer, json);
}

}

std::string VegetationCalibrationEvidenceBundleSerializationService::serialize(
	const VegetationCalibrationEvidenceBundle &bundle) const
{
	Json::Value document(Json::objectValue);
	document["payload_sha256"] = bundle.payloadSha256();
	document["payload"] = payload_json(bundle);
	return compact_json(document);
}

std::string
VegetationCalibrationEvidenceBundleSerializationService::canonicalPayloadJson(
	const VegetationCalibrationEvidenceBundle &bundle) const
{
	return compact_json(payload_json(bundle));
}
