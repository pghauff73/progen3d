#include "vegetation/service/VegetationCanopyObservationAdapter.h"

#include "VegetationMeasuredSourceJsonDocumentReader.h"
#include "vegetation/model/VegetationCanopyObservationRecord.h"
#include "vegetation/service/VegetationMeasuredCalibrationMeasurementFactory.h"
#include "vegetation/service/VegetationMeasuredEvidenceBundleFactory.h"
#include "vegetation/service/VegetationMeasuredSourceArtifactValidationService.h"

#include <optional>
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
	"application/vnd.progen3d.canopy-observation+json";
constexpr const char *payload_schema = "ProGen3D-CanopyObservation-v1";

void add_issue(
	std::vector<Issue> &issues,
	IssueCode code,
	const std::string &record_identifier,
	const std::string &message)
{
	issues.emplace_back(code, record_identifier, message);
}

}

VegetationMeasuredSourceAdapterReport VegetationCanopyObservationAdapter::adapt(
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
			"Canopy observation payload schema is unsupported.");
	}
	const std::optional<PlantArchitecture> architecture =
		reader.requiredArchitecture(
			*document, "plant_architecture", "plant_architecture");
	if (architecture.has_value() && *architecture != subject_scope.architecture()) {
		add_issue(
			issues, IssueCode::ArchitectureMismatch, std::string(),
			"Canopy observation architecture does not match the subject.");
	}
	const std::string observation_identifier = reader.requiredString(
		*document, "observation_identifier", "observation_identifier");
	const std::string point_cloud_identifier = reader.optionalString(
		*document, "point_cloud_identifier", "point_cloud_identifier");
	const std::optional<double> leaf_area_index =
		reader.requiredNormalizedMeasurement(
			*document, "leaf_area_index", "leaf_area_index",
			VegetationMeasurementQuantityKind::Dimensionless,
			observation_identifier);
	const std::optional<double> gap_fraction =
		reader.requiredNormalizedMeasurement(
			*document, "crown_gap_fraction", "crown_gap_fraction",
			VegetationMeasurementQuantityKind::Fraction,
			observation_identifier);
	const std::optional<double> leaf_inclination =
		reader.requiredNormalizedMeasurement(
			*document, "mean_leaf_inclination",
			"mean_leaf_inclination",
			VegetationMeasurementQuantityKind::AngleDegrees,
			observation_identifier);
	if (leaf_area_index.has_value() && *leaf_area_index <= 0.0) {
		add_issue(
			issues, IssueCode::InvalidValueRange, observation_identifier,
			"Leaf area index must be positive.");
	}
	if (gap_fraction.has_value() &&
	    (*gap_fraction <= 0.0 || *gap_fraction > 1.0)) {
		add_issue(
			issues, IssueCode::InvalidValueRange, observation_identifier,
			"Crown gap fraction must be greater than zero and at most one.");
	}
	if (leaf_inclination.has_value() &&
	    (*leaf_inclination < 0.0 || *leaf_inclination > 90.0)) {
		add_issue(
			issues, IssueCode::InvalidValueRange, observation_identifier,
			"Mean leaf inclination must be between zero and ninety degrees.");
	}
	if (!issues.empty() || !leaf_area_index.has_value() ||
	    !gap_fraction.has_value() || !leaf_inclination.has_value()) {
		if (!observation_identifier.empty()) {
			observations.emplace_back(
				observation_identifier, Disposition::Rejected,
				"Canopy observation was rejected by optical validation.");
		}
		return VegetationMeasuredSourceAdapterReport(
			std::move(issues), std::move(observations), std::nullopt);
	}

	const VegetationCanopyObservationRecord record(
		observation_identifier,
		*leaf_area_index,
		*gap_fraction,
		*leaf_inclination,
		point_cloud_identifier);
	observations.emplace_back(
		record.observationIdentifier(), Disposition::Accepted,
		"Canopy observation produced calibrated optical measurements.");
	if (!record.pointCloudIdentifier().empty()) {
		observations.emplace_back(
			record.pointCloudIdentifier(), Disposition::Deferred,
			"Point-cloud identity is retained by the source artifact; the v1 evidence bundle has no separate point-cloud measurement kind.");
	}
	const VegetationMeasuredCalibrationMeasurementFactory measurements;
	std::vector<VegetationCalibrationMeasurement> output = {
		measurements.createDecimal(
			MeasurementKind::LeafAreaIndex,
			record.leafAreaIndex(),
			"dimensionless",
			subject_scope,
			artifact),
		measurements.createDecimal(
			MeasurementKind::CrownGapFraction,
			record.crownGapFraction(),
			"fraction",
			subject_scope,
			artifact),
		measurements.createDecimal(
			MeasurementKind::MeanLeafInclinationDegrees,
			record.meanLeafInclinationDegrees(),
			"degree",
			subject_scope,
			artifact),
	};
	return VegetationMeasuredSourceAdapterReport(
		std::move(issues),
		std::move(observations),
		VegetationMeasuredEvidenceBundleFactory().create(
			subject_scope, artifact, "canopy-observation-v1", std::move(output)));
}
