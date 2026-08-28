#include "vegetation/service/VegetationBiomechanicalMaterialTestAdapter.h"

#include "VegetationMeasuredSourceJsonDocumentReader.h"
#include "vegetation/model/VegetationBiomechanicalMaterialTestRecord.h"
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
	"application/vnd.progen3d.biomechanical-test+json";
constexpr const char *payload_schema =
	"ProGen3D-BiomechanicalMaterialTest-v1";

void add_issue(
	std::vector<Issue> &issues,
	IssueCode code,
	const std::string &record_identifier,
	const std::string &message)
{
	issues.emplace_back(code, record_identifier, message);
}

}

VegetationMeasuredSourceAdapterReport
VegetationBiomechanicalMaterialTestAdapter::adapt(
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
			"Biomechanical test payload schema is unsupported.");
	}
	const std::optional<PlantArchitecture> architecture =
		reader.requiredArchitecture(
			*document, "plant_architecture", "plant_architecture");
	if (architecture.has_value() && *architecture != subject_scope.architecture()) {
		add_issue(
			issues, IssueCode::ArchitectureMismatch, std::string(),
			"Biomechanical test architecture does not match the subject.");
	}
	const std::string test_identifier = reader.requiredString(
		*document, "test_identifier", "test_identifier");
	const std::optional<double> density =
		reader.requiredNormalizedMeasurement(
			*document, "mass_density", "mass_density",
			VegetationMeasurementQuantityKind::MassDensityKilogramsPerCubicMetre,
			test_identifier);
	const std::optional<double> elastic_modulus =
		reader.requiredNormalizedMeasurement(
			*document, "elastic_modulus", "elastic_modulus",
			VegetationMeasurementQuantityKind::PressurePascals,
			test_identifier);
	const std::optional<double> damping_ratio =
		reader.requiredNormalizedMeasurement(
			*document, "damping_ratio", "damping_ratio",
			VegetationMeasurementQuantityKind::Fraction,
			test_identifier);
	const std::optional<double> drag_coefficient =
		reader.requiredNormalizedMeasurement(
			*document, "drag_coefficient", "drag_coefficient",
			VegetationMeasurementQuantityKind::Dimensionless,
			test_identifier);
	if (density.has_value() && *density <= 0.0) {
		add_issue(
			issues, IssueCode::InvalidValueRange, test_identifier,
			"Biomechanical mass density must be positive.");
	}
	if (elastic_modulus.has_value() && *elastic_modulus <= 0.0) {
		add_issue(
			issues, IssueCode::InvalidValueRange, test_identifier,
			"Biomechanical elastic modulus must be positive.");
	}
	if (damping_ratio.has_value() &&
	    (*damping_ratio < 0.0 || *damping_ratio > 1.0)) {
		add_issue(
			issues, IssueCode::InvalidValueRange, test_identifier,
			"Biomechanical damping ratio must be between zero and one.");
	}
	if (drag_coefficient.has_value() && *drag_coefficient <= 0.0) {
		add_issue(
			issues, IssueCode::InvalidValueRange, test_identifier,
			"Biomechanical drag coefficient must be positive.");
	}
	if (!issues.empty() || !density.has_value() ||
	    !elastic_modulus.has_value() || !damping_ratio.has_value() ||
	    !drag_coefficient.has_value()) {
		if (!test_identifier.empty()) {
			observations.emplace_back(
				test_identifier, Disposition::Rejected,
				"Biomechanical test was rejected by material validation.");
		}
		return VegetationMeasuredSourceAdapterReport(
			std::move(issues), std::move(observations), std::nullopt);
	}

	const VegetationBiomechanicalMaterialTestRecord record(
		test_identifier,
		*density,
		*elastic_modulus,
		*damping_ratio,
		*drag_coefficient);
	observations.emplace_back(
		record.testIdentifier(), Disposition::Accepted,
		"Biomechanical test produced calibrated material measurements.");
	const VegetationMeasuredCalibrationMeasurementFactory measurements;
	std::vector<VegetationCalibrationMeasurement> output = {
		measurements.createDecimal(
			MeasurementKind::MassDensityKilogramsPerCubicMetre,
			record.massDensityKilogramsPerCubicMetre(),
			"kg/m3",
			subject_scope,
			artifact),
		measurements.createDecimal(
			MeasurementKind::ElasticModulusPascals,
			record.elasticModulusPascals(),
			"Pa",
			subject_scope,
			artifact),
		measurements.createDecimal(
			MeasurementKind::DampingRatio,
			record.dampingRatio(),
			"dimensionless",
			subject_scope,
			artifact),
		measurements.createDecimal(
			MeasurementKind::DragCoefficient,
			record.dragCoefficient(),
			"dimensionless",
			subject_scope,
			artifact),
	};
	return VegetationMeasuredSourceAdapterReport(
		std::move(issues),
		std::move(observations),
		VegetationMeasuredEvidenceBundleFactory().create(
			subject_scope, artifact, "biomechanical-test-v1", std::move(output)));
}
