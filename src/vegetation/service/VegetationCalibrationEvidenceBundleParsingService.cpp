#include "vegetation/service/VegetationCalibrationEvidenceBundleParsingService.h"

#include <json/json.h>

#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

using ParsingCode = VegetationCalibrationEvidenceBundleParsingCode;
using ParsingDiagnostic =
	VegetationCalibrationEvidenceBundleParsingDiagnostic;

class VegetationCalibrationEvidenceJsonDocumentReader
{
public:
	VegetationCalibrationEvidenceBundleParsingResult read(
		const std::string &json_text)
	{
		Json::CharReaderBuilder builder;
		builder["collectComments"] = false;
		std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
		Json::Value document;
		std::string errors;
		if (!reader->parse(
			    json_text.data(), json_text.data() + json_text.size(),
			    &document, &errors)) {
			diagnostics_.emplace_back(
				ParsingCode::InvalidJson,
				"Calibration evidence JSON is invalid: " + errors);
			return result(std::nullopt);
		}
		if (!document.isObject() || !document["payload"].isObject()) {
			diagnostics_.emplace_back(
				ParsingCode::MissingPayload,
				"Calibration evidence JSON requires an object payload.");
			return result(std::nullopt);
		}

		const Json::Value &payload = document["payload"];
		const std::string schema_version = required_string(
			payload, "schema_version", "payload.schema_version");
		const std::string bundle_identifier = required_string(
			payload, "bundle_identifier", "payload.bundle_identifier");
		const std::string payload_sha256 = required_string(
			document, "payload_sha256", "payload_sha256");
		const std::optional<VegetationCalibrationSubjectScope> subject_scope =
			read_subject_scope(payload["subject_scope"]);
		const std::string coordinate_system = required_string(
			payload, "coordinate_system", "payload.coordinate_system");
		const std::string unit_system = required_string(
			payload, "unit_system", "payload.unit_system");
		const std::string acquisition_method = required_string(
			payload, "acquisition_method", "payload.acquisition_method");
		const std::string uncertainty_statement = required_string(
			payload, "uncertainty_statement",
			"payload.uncertainty_statement");
		std::vector<VegetationCalibrationSourceReference> source_references =
			read_source_references(payload["source_references"]);
		std::vector<VegetationCalibrationMeasurement> measurements =
			read_measurements(payload["measurements"]);

		if (!diagnostics_.empty() || !subject_scope.has_value()) {
			return result(std::nullopt);
		}
		return result(VegetationCalibrationEvidenceBundle(
			schema_version,
			bundle_identifier,
			*subject_scope,
			coordinate_system,
			unit_system,
			acquisition_method,
			uncertainty_statement,
			std::move(source_references),
			std::move(measurements),
			payload_sha256));
	}

private:
	VegetationCalibrationEvidenceBundleParsingResult result(
		std::optional<VegetationCalibrationEvidenceBundle> bundle)
	{
		return VegetationCalibrationEvidenceBundleParsingResult(
			std::move(bundle), std::move(diagnostics_));
	}

	std::string required_string(
		const Json::Value &object,
		const char *field_name,
		const std::string &field_path)
	{
		if (!object.isObject() || !object[field_name].isString()) {
			diagnostics_.emplace_back(
				ParsingCode::MissingRequiredField,
				"Calibration evidence JSON requires string field " +
					field_path + ".");
			return std::string();
		}
		return object[field_name].asString();
	}

	std::optional<VegetationCalibrationSubjectScope> read_subject_scope(
		const Json::Value &json)
	{
		if (!json.isObject()) {
			diagnostics_.emplace_back(
				ParsingCode::MissingRequiredField,
				"Calibration evidence JSON requires payload.subject_scope.");
			return std::nullopt;
		}
		const std::string kind_name = required_string(
			json, "kind", "payload.subject_scope.kind");
		const std::string architecture_name = required_string(
			json, "plant_architecture",
			"payload.subject_scope.plant_architecture");
		const std::optional<VegetationCalibrationSubjectScopeKind> kind =
			vegetationCalibrationSubjectScopeKindFromName(kind_name);
		if (!kind.has_value()) {
			diagnostics_.emplace_back(
				ParsingCode::UnsupportedSubjectScope,
				"Unsupported vegetation calibration subject scope: " + kind_name);
		}
		const std::optional<PlantArchitecture> architecture =
			plantArchitectureFromName(architecture_name);
		if (!architecture.has_value()) {
			diagnostics_.emplace_back(
				ParsingCode::UnsupportedPlantArchitecture,
				"Unsupported vegetation plant architecture: " +
					architecture_name);
		}
		if (!kind.has_value() || !architecture.has_value()) return std::nullopt;
		return VegetationCalibrationSubjectScope(
			*kind,
			*architecture,
			required_string(
				json, "subject_identifier",
				"payload.subject_scope.subject_identifier"),
			required_string(
				json, "scientific_name",
				"payload.subject_scope.scientific_name"),
			required_string(
				json, "cultivar_name",
				"payload.subject_scope.cultivar_name"),
			required_string(
				json, "specimen_identifier",
				"payload.subject_scope.specimen_identifier"));
	}

	std::vector<VegetationCalibrationSourceReference> read_source_references(
		const Json::Value &json)
	{
		std::vector<VegetationCalibrationSourceReference> references;
		if (!json.isArray()) {
			diagnostics_.emplace_back(
				ParsingCode::MissingRequiredField,
				"Calibration evidence JSON requires a source_references array.");
			return references;
		}
		for (Json::ArrayIndex index = 0; index < json.size(); ++index) {
			const Json::Value &source = json[index];
			references.emplace_back(
				required_string(
					source, "source_identifier", "source_references.source_identifier"),
				required_string(source, "citation", "source_references.citation"),
				required_string(source, "locator", "source_references.locator"),
				required_string(source, "sha256", "source_references.sha256"));
		}
		return references;
	}

	std::vector<std::string> read_string_array(
		const Json::Value &json,
		const std::string &field_path)
	{
		std::vector<std::string> values;
		if (!json.isArray()) {
			diagnostics_.emplace_back(
				ParsingCode::InvalidMeasurementValue,
				field_path + " must be an array of strings.");
			return values;
		}
		for (Json::ArrayIndex index = 0; index < json.size(); ++index) {
			if (!json[index].isString()) {
				diagnostics_.emplace_back(
					ParsingCode::InvalidMeasurementValue,
					field_path + " must contain only strings.");
				continue;
			}
			values.push_back(json[index].asString());
		}
		return values;
	}

	std::optional<VegetationCalibrationMeasurementValue> read_measurement_value(
		VegetationCalibrationMeasurementValueKind value_kind,
		const Json::Value &json)
	{
		switch (value_kind) {
		case VegetationCalibrationMeasurementValueKind::Decimal:
			if (json.isNumeric()) {
				return VegetationCalibrationMeasurementValue(json.asDouble());
			}
			break;
		case VegetationCalibrationMeasurementValueKind::Count:
			if (json.isUInt64()) {
				return VegetationCalibrationMeasurementValue(
					static_cast<std::size_t>(json.asUInt64()));
			}
			break;
		case VegetationCalibrationMeasurementValueKind::Text:
			if (json.isString()) {
				return VegetationCalibrationMeasurementValue(json.asString());
			}
			break;
		case VegetationCalibrationMeasurementValueKind::IdentifierList:
			if (json.isArray()) {
				return VegetationCalibrationMeasurementValue(
					read_string_array(json, "measurements.value"));
			}
			break;
		}
		diagnostics_.emplace_back(
			ParsingCode::InvalidMeasurementValue,
			"Calibration evidence measurement value does not match value_kind.");
		return std::nullopt;
	}

	std::vector<VegetationCalibrationMeasurement> read_measurements(
		const Json::Value &json)
	{
		std::vector<VegetationCalibrationMeasurement> measurements;
		if (!json.isArray()) {
			diagnostics_.emplace_back(
				ParsingCode::MissingRequiredField,
				"Calibration evidence JSON requires a measurements array.");
			return measurements;
		}
		for (Json::ArrayIndex index = 0; index < json.size(); ++index) {
			const Json::Value &measurement = json[index];
			const std::string kind_name = required_string(
				measurement, "kind", "measurements.kind");
			const std::optional<VegetationCalibrationMeasurementKind> kind =
				vegetationCalibrationMeasurementKindFromName(kind_name);
			if (!kind.has_value()) {
				diagnostics_.emplace_back(
					ParsingCode::UnsupportedMeasurementKind,
					"Unsupported vegetation calibration measurement kind: " +
						kind_name);
				continue;
			}
			const std::string value_kind_name = required_string(
				measurement, "value_kind", "measurements.value_kind");
			const std::optional<VegetationCalibrationMeasurementValueKind>
				value_kind = vegetationCalibrationMeasurementValueKindFromName(
					value_kind_name);
			if (!value_kind.has_value()) {
				diagnostics_.emplace_back(
					ParsingCode::UnsupportedMeasurementValueKind,
					"Unsupported vegetation calibration value kind: " +
						value_kind_name);
				continue;
			}
			const std::optional<VegetationCalibrationMeasurementValue> value =
				read_measurement_value(*value_kind, measurement["value"]);
			if (!value.has_value()) continue;
			measurements.emplace_back(
				*kind,
				required_string(measurement, "domain", "measurements.domain"),
				*value,
				required_string(measurement, "unit", "measurements.unit"),
				required_string(
					measurement, "observation_scope",
					"measurements.observation_scope"),
				required_string(
					measurement, "uncertainty_statement",
					"measurements.uncertainty_statement"),
				read_string_array(
					measurement["evidence_source_identifiers"],
					"measurements.evidence_source_identifiers"));
		}
		return measurements;
	}

	std::vector<ParsingDiagnostic> diagnostics_;
};

}

VegetationCalibrationEvidenceBundleParsingResult
VegetationCalibrationEvidenceBundleParsingService::parse(
	const std::string &json_text) const
{
	return VegetationCalibrationEvidenceJsonDocumentReader().read(json_text);
}
