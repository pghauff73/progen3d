#pragma once

#include "vegetation/model/PlantArchitecture.h"
#include "vegetation/model/VegetationMeasuredPoint3d.h"
#include "vegetation/model/VegetationMeasuredSourceAdapterReport.h"
#include "vegetation/model/VegetationMeasurementUnitNormalizationResult.h"

#include <json/json.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

class VegetationMeasuredSourceJsonDocumentReader
{
public:
	explicit VegetationMeasuredSourceJsonDocumentReader(
		std::vector<VegetationMeasuredSourceAdapterIssue> &issues)
		: issues_(issues)
	{
	}

	std::optional<Json::Value> readObject(const std::string &json_text);
	std::string requiredString(
		const Json::Value &object,
		const char *field_name,
		const std::string &field_path,
		const std::string &record_identifier = std::string());
	std::string optionalString(
		const Json::Value &object,
		const char *field_name,
		const std::string &field_path,
		const std::string &record_identifier = std::string());
	std::optional<double> requiredDecimal(
		const Json::Value &object,
		const char *field_name,
		const std::string &field_path,
		const std::string &record_identifier = std::string());
	std::optional<std::size_t> requiredCount(
		const Json::Value &object,
		const char *field_name,
		const std::string &field_path,
		const std::string &record_identifier = std::string());
	const Json::Value *requiredArray(
		const Json::Value &object,
		const char *field_name,
		const std::string &field_path,
		const std::string &record_identifier = std::string());
	std::optional<VegetationMeasuredPoint3d> requiredPoint(
		const Json::Value &object,
		const char *field_name,
		const std::string &field_path,
		const std::string &length_unit,
		const std::string &coordinate_system,
		const std::string &record_identifier = std::string());
	std::optional<double> requiredNormalizedMeasurement(
		const Json::Value &object,
		const char *field_name,
		const std::string &field_path,
		VegetationMeasurementQuantityKind quantity_kind,
		const std::string &record_identifier = std::string());
	std::optional<PlantArchitecture> requiredArchitecture(
		const Json::Value &object,
		const char *field_name,
		const std::string &field_path);

private:
	void addIssue(
		VegetationMeasuredSourceAdapterIssueCode code,
		const std::string &record_identifier,
		const std::string &message);

	std::vector<VegetationMeasuredSourceAdapterIssue> &issues_;
};
