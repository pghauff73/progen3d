#include "vegetation/service/VegetationPhenologyObservationSeriesAdapter.h"

#include "VegetationMeasuredSourceJsonDocumentReader.h"
#include "vegetation/model/VegetationPhenologyObservationRecord.h"
#include "vegetation/service/VegetationMeasuredCalibrationMeasurementFactory.h"
#include "vegetation/service/VegetationMeasuredEvidenceBundleFactory.h"
#include "vegetation/service/VegetationMeasuredSourceArtifactValidationService.h"

#include <cctype>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

using Issue = VegetationMeasuredSourceAdapterIssue;
using IssueCode = VegetationMeasuredSourceAdapterIssueCode;
using Observation = VegetationMeasuredSourceRecordObservation;
using Disposition = VegetationMeasuredSourceRecordDisposition;
using MeasurementKind = VegetationCalibrationMeasurementKind;

constexpr const char *media_type =
	"application/vnd.progen3d.phenology-series+json";
constexpr const char *payload_schema =
	"ProGen3D-PhenologyObservationSeries-v1";

void add_issue(
	std::vector<Issue> &issues,
	IssueCode code,
	const std::string &record_identifier,
	const std::string &message)
{
	issues.emplace_back(code, record_identifier, message);
}

bool is_leap_year(int year)
{
	return year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
}

bool is_iso_calendar_date(const std::string &date)
{
	if (date.size() != 10u || date[4] != '-' || date[7] != '-') return false;
	for (std::size_t index = 0u; index < date.size(); ++index) {
		if (index == 4u || index == 7u) continue;
		if (std::isdigit(static_cast<unsigned char>(date[index])) == 0) return false;
	}
	const int year = std::stoi(date.substr(0u, 4u));
	const int month = std::stoi(date.substr(5u, 2u));
	const int day = std::stoi(date.substr(8u, 2u));
	if (year <= 0 || month < 1 || month > 12 || day < 1) return false;
	const int month_lengths[] = {
		31, is_leap_year(year) ? 29 : 28, 31, 30, 31, 30,
		31, 31, 30, 31, 30, 31,
	};
	return day <= month_lengths[month - 1];
}

}

VegetationMeasuredSourceAdapterReport
VegetationPhenologyObservationSeriesAdapter::adapt(
	const VegetationMeasuredSourceArtifact &artifact,
	const VegetationCalibrationSubjectScope &subject_scope) const
{
	std::vector<Issue> issues =
		VegetationMeasuredSourceArtifactValidationService().validate(
			artifact, media_type);
	std::vector<Observation> observations;
	if (!issues.empty()) {
		return VegetationMeasuredSourceAdapterReport(
			std::move(issues), std::move(observations), std::nullopt);
	}

	VegetationMeasuredSourceJsonDocumentReader reader(issues);
	const std::optional<Json::Value> document =
		reader.readObject(artifact.sourcePayload());
	if (!document.has_value()) {
		return VegetationMeasuredSourceAdapterReport(
			std::move(issues), std::move(observations), std::nullopt);
	}
	const std::string schema = reader.requiredString(
		*document, "schema_version", "schema_version");
	if (!schema.empty() && schema != payload_schema) {
		add_issue(
			issues, IssueCode::UnsupportedPayloadSchema, std::string(),
			"Phenology observation payload schema is unsupported.");
	}
	const std::optional<PlantArchitecture> architecture =
		reader.requiredArchitecture(
			*document, "plant_architecture", "plant_architecture");
	if (architecture.has_value() && *architecture != subject_scope.architecture()) {
		add_issue(
			issues, IssueCode::ArchitectureMismatch, std::string(),
			"Phenology series architecture does not match the subject.");
	}
	const std::string series_identifier = reader.requiredString(
		*document, "series_identifier", "series_identifier");
	const Json::Value *source_observations = reader.requiredArray(
		*document, "observations", "observations");

	std::vector<VegetationPhenologyObservationRecord> records;
	std::set<std::string> dates;
	std::set<std::string> stage_identifiers;
	std::string prior_date;
	if (source_observations != nullptr) {
		for (Json::ArrayIndex index = 0u; index < source_observations->size(); ++index) {
			const Json::Value &source_observation = (*source_observations)[index];
			const std::string path =
				"observations[" + std::to_string(index) + "]";
			const std::string date = reader.requiredString(
				source_observation, "date", path + ".date");
			const std::string stage_identifier = reader.requiredString(
				source_observation, "development_stage_identifier",
				path + ".development_stage_identifier", date);
			bool accepted_record = !date.empty() && !stage_identifier.empty();
			if (!date.empty() && !is_iso_calendar_date(date)) {
				accepted_record = false;
				add_issue(
					issues, IssueCode::InvalidObservationOrder, date,
					"Phenology observation date must be a valid ISO calendar date.");
			}
			if (!date.empty() && !dates.insert(date).second) {
				accepted_record = false;
				add_issue(
					issues, IssueCode::DuplicateObservation, date,
					"Phenology series contains a duplicate observation date.");
			}
			if (!prior_date.empty() && !date.empty() && date <= prior_date) {
				accepted_record = false;
				add_issue(
					issues, IssueCode::InvalidObservationOrder, date,
					"Phenology observations must be in strictly increasing date order.");
			}
			if (!date.empty()) prior_date = date;
			if (accepted_record) {
				records.emplace_back(date, stage_identifier);
				stage_identifiers.insert(stage_identifier);
			}
			observations.emplace_back(
				date.empty() ? path : date,
				accepted_record ? Disposition::Accepted : Disposition::Rejected,
				accepted_record
					? "Phenology observation contributed to the seasonal schedule."
					: "Phenology observation was rejected by chronology validation.");
		}
	}
	if (records.size() < 2u || stage_identifiers.size() < 2u) {
		add_issue(
			issues, IssueCode::EmptyObservationSeries, series_identifier,
			"Phenology series requires at least two dated observations and two development stages.");
	}
	if (!issues.empty()) {
		return VegetationMeasuredSourceAdapterReport(
			std::move(issues), std::move(observations), std::nullopt);
	}

	const VegetationMeasuredCalibrationMeasurementFactory measurements;
	std::vector<VegetationCalibrationMeasurement> output = {
		measurements.createText(
			MeasurementKind::SeasonalScheduleArtifactIdentifier,
			series_identifier,
			"identifier",
			subject_scope,
			artifact),
	};
	return VegetationMeasuredSourceAdapterReport(
		std::move(issues),
		std::move(observations),
		VegetationMeasuredEvidenceBundleFactory().create(
			subject_scope, artifact, "phenology-series-v1", std::move(output)));
}
