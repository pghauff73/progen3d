#include "VegetationMeasuredSourceJsonDocumentReader.h"

#include "vegetation/service/VegetationMeasurementUnitNormalizationService.h"
#include "vegetation/service/VegetationMeasuredCoordinateNormalizationService.h"

#include <cmath>
#include <memory>
#include <optional>
#include <string>

std::optional<Json::Value>
VegetationMeasuredSourceJsonDocumentReader::readObject(
	const std::string &json_text)
{
	Json::CharReaderBuilder builder;
	builder["collectComments"] = false;
	std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	Json::Value document;
	std::string errors;
	if (!reader->parse(
		    json_text.data(), json_text.data() + json_text.size(),
		    &document, &errors) ||
	    !document.isObject()) {
		addIssue(
			VegetationMeasuredSourceAdapterIssueCode::MalformedSourcePayload,
			std::string(),
			"Measured source payload is not a valid JSON object: " + errors);
		return std::nullopt;
	}
	return document;
}

std::string VegetationMeasuredSourceJsonDocumentReader::requiredString(
	const Json::Value &object,
	const char *field_name,
	const std::string &field_path,
	const std::string &record_identifier)
{
	if (!object.isObject() || !object[field_name].isString() ||
	    object[field_name].asString().empty()) {
		addIssue(
			VegetationMeasuredSourceAdapterIssueCode::MissingRequiredField,
			record_identifier,
			"Measured source payload requires string field " + field_path + ".");
		return std::string();
	}
	return object[field_name].asString();
}

std::string VegetationMeasuredSourceJsonDocumentReader::optionalString(
	const Json::Value &object,
	const char *field_name,
	const std::string &field_path,
	const std::string &record_identifier)
{
	if (!object.isObject() || object[field_name].isNull()) return std::string();
	if (!object[field_name].isString()) {
		addIssue(
			VegetationMeasuredSourceAdapterIssueCode::MissingRequiredField,
			record_identifier,
			"Measured source optional field must be a string when present: " +
				field_path + ".");
		return std::string();
	}
	return object[field_name].asString();
}

std::optional<double>
VegetationMeasuredSourceJsonDocumentReader::requiredDecimal(
	const Json::Value &object,
	const char *field_name,
	const std::string &field_path,
	const std::string &record_identifier)
{
	if (!object.isObject() || !object[field_name].isNumeric()) {
		addIssue(
			VegetationMeasuredSourceAdapterIssueCode::MissingRequiredField,
			record_identifier,
			"Measured source payload requires numeric field " + field_path + ".");
		return std::nullopt;
	}
	const double value = object[field_name].asDouble();
	if (!std::isfinite(value)) {
		addIssue(
			VegetationMeasuredSourceAdapterIssueCode::NonFiniteValue,
			record_identifier,
			"Measured source field is not finite: " + field_path + ".");
		return std::nullopt;
	}
	return value;
}

std::optional<std::size_t>
VegetationMeasuredSourceJsonDocumentReader::requiredCount(
	const Json::Value &object,
	const char *field_name,
	const std::string &field_path,
	const std::string &record_identifier)
{
	if (!object.isObject() || !object[field_name].isUInt64()) {
		addIssue(
			VegetationMeasuredSourceAdapterIssueCode::MissingRequiredField,
			record_identifier,
			"Measured source payload requires unsigned count field " + field_path +
				".");
		return std::nullopt;
	}
	return static_cast<std::size_t>(object[field_name].asUInt64());
}

const Json::Value *VegetationMeasuredSourceJsonDocumentReader::requiredArray(
	const Json::Value &object,
	const char *field_name,
	const std::string &field_path,
	const std::string &record_identifier)
{
	if (!object.isObject() || !object[field_name].isArray()) {
		addIssue(
			VegetationMeasuredSourceAdapterIssueCode::MissingRequiredField,
			record_identifier,
			"Measured source payload requires array field " + field_path + ".");
		return nullptr;
	}
	return &object[field_name];
}

std::optional<VegetationMeasuredPoint3d>
VegetationMeasuredSourceJsonDocumentReader::requiredPoint(
	const Json::Value &object,
	const char *field_name,
	const std::string &field_path,
	const std::string &length_unit,
	const std::string &coordinate_system,
	const std::string &record_identifier)
{
	const Json::Value *array =
		requiredArray(object, field_name, field_path, record_identifier);
	if (array == nullptr) return std::nullopt;
	if (array->size() != 3u || !(*array)[0u].isNumeric() ||
	    !(*array)[1u].isNumeric() || !(*array)[2u].isNumeric()) {
		addIssue(
			VegetationMeasuredSourceAdapterIssueCode::MissingRequiredField,
			record_identifier,
			"Measured source point requires exactly three numeric coordinates: " +
				field_path + ".");
		return std::nullopt;
	}
	const VegetationMeasurementUnitNormalizationService normalization;
	const auto x = normalization.normalize(
		(*array)[0u].asDouble(), length_unit,
		VegetationMeasurementQuantityKind::LengthMetres);
	const auto y = normalization.normalize(
		(*array)[1u].asDouble(), length_unit,
		VegetationMeasurementQuantityKind::LengthMetres);
	const auto z = normalization.normalize(
		(*array)[2u].asDouble(), length_unit,
		VegetationMeasurementQuantityKind::LengthMetres);
	if (!x.succeeded() || !y.succeeded() || !z.succeeded()) {
		addIssue(
			VegetationMeasuredSourceAdapterIssueCode::UnsupportedUnit,
			record_identifier,
			"Measured source point uses an unsupported or non-finite length unit: " +
				field_path + ".");
		return std::nullopt;
	}
	const VegetationMeasuredCoordinateNormalizationResult normalized_point =
		VegetationMeasuredCoordinateNormalizationService().normalize(
			VegetationMeasuredPoint3d(
				*x.normalizedValue(), *y.normalizedValue(), *z.normalizedValue()),
			coordinate_system);
	if (!normalized_point.succeeded()) {
		addIssue(
			VegetationMeasuredSourceAdapterIssueCode::UnsupportedCoordinateSystem,
			record_identifier,
			"Measured source point cannot be normalized: " + field_path + ". " +
				normalized_point.rejectionReason());
		return std::nullopt;
	}
	return *normalized_point.normalizedPoint();
}

std::optional<double>
VegetationMeasuredSourceJsonDocumentReader::requiredNormalizedMeasurement(
	const Json::Value &object,
	const char *field_name,
	const std::string &field_path,
	VegetationMeasurementQuantityKind quantity_kind,
	const std::string &record_identifier)
{
	const Json::Value &measurement = object[field_name];
	if (!measurement.isObject()) {
		addIssue(
			VegetationMeasuredSourceAdapterIssueCode::MissingRequiredField,
			record_identifier,
			"Measured source payload requires measurement object " + field_path +
				".");
		return std::nullopt;
	}
	const std::optional<double> value = requiredDecimal(
		measurement, "value", field_path + ".value", record_identifier);
	const std::string unit = requiredString(
		measurement, "unit", field_path + ".unit", record_identifier);
	if (!value.has_value() || unit.empty()) return std::nullopt;
	const VegetationMeasurementUnitNormalizationResult result =
		VegetationMeasurementUnitNormalizationService().normalize(
			*value, unit, quantity_kind);
	if (!result.succeeded()) {
		addIssue(
			VegetationMeasuredSourceAdapterIssueCode::UnsupportedUnit,
			record_identifier,
			"Measured source field cannot be normalized: " + field_path + ". " +
				result.rejectionReason());
		return std::nullopt;
	}
	return result.normalizedValue();
}

std::optional<PlantArchitecture>
VegetationMeasuredSourceJsonDocumentReader::requiredArchitecture(
	const Json::Value &object,
	const char *field_name,
	const std::string &field_path)
{
	const std::string name =
		requiredString(object, field_name, field_path, std::string());
	if (name.empty()) return std::nullopt;
	const std::optional<PlantArchitecture> architecture =
		plantArchitectureFromName(name);
	if (!architecture.has_value()) {
		addIssue(
			VegetationMeasuredSourceAdapterIssueCode::ArchitectureMismatch,
			std::string(),
			"Measured source payload declares an unsupported plant architecture.");
	}
	return architecture;
}

void VegetationMeasuredSourceJsonDocumentReader::addIssue(
	VegetationMeasuredSourceAdapterIssueCode code,
	const std::string &record_identifier,
	const std::string &message)
{
	issues_.emplace_back(code, record_identifier, message);
}
