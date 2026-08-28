#include "vegetation/service/VegetationQuantitativeStructureModelAdapter.h"

#include "VegetationMeasuredSourceJsonDocumentReader.h"
#include "vegetation/model/VegetationQuantitativeStructureModelRecord.h"
#include "vegetation/service/VegetationMeasuredCalibrationMeasurementFactory.h"
#include "vegetation/service/VegetationMeasuredEvidenceBundleFactory.h"
#include "vegetation/service/VegetationMeasuredSourceArtifactValidationService.h"
#include "vegetation/service/VegetationMeasurementUnitNormalizationService.h"

#include <algorithm>
#include <cmath>
#include <map>
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
using CylinderRecord = VegetationQuantitativeStructureModelCylinderRecord;

constexpr const char *media_type = "application/vnd.progen3d.qsm+json";
constexpr const char *payload_schema = "ProGen3D-QuantitativeStructureModel-v1";

double distance(
	const VegetationMeasuredPoint3d &start,
	const VegetationMeasuredPoint3d &end)
{
	const double x = end.x() - start.x();
	const double y = end.y() - start.y();
	const double z = end.z() - start.z();
	return std::sqrt(x * x + y * y + z * z);
}

void add_issue(
	std::vector<Issue> &issues,
	IssueCode code,
	const std::string &record_identifier,
	const std::string &message)
{
	issues.emplace_back(code, record_identifier, message);
}

std::set<std::string> graph_rejected_identifiers(
	const std::vector<CylinderRecord> &records,
	std::vector<Issue> &issues)
{
	std::set<std::string> rejected;
	std::map<std::string, const CylinderRecord *> records_by_identifier;
	std::size_t root_count = 0u;
	for (const CylinderRecord &record : records) {
		records_by_identifier.emplace(record.cylinderIdentifier(), &record);
		if (record.parentCylinderIdentifier().empty()) ++root_count;
	}
	if (root_count != 1u) {
		add_issue(
			issues, IssueCode::IncompleteGraph, std::string(),
			"QSM cylinder graph requires exactly one root cylinder.");
	}
	for (const CylinderRecord &record : records) {
		if (record.parentCylinderIdentifier().empty()) continue;
		const auto parent = records_by_identifier.find(
			record.parentCylinderIdentifier());
		if (parent == records_by_identifier.end()) {
			rejected.insert(record.cylinderIdentifier());
			add_issue(
				issues, IssueCode::InvalidParentReference,
				record.cylinderIdentifier(),
				"QSM cylinder parent identifier does not exist.");
			continue;
		}
		if (record.branchOrder() < parent->second->branchOrder() ||
		    record.branchOrder() > parent->second->branchOrder() + 1u) {
			rejected.insert(record.cylinderIdentifier());
			add_issue(
				issues, IssueCode::InvalidValueRange,
				record.cylinderIdentifier(),
				"QSM branch order must equal or increment its parent order by one.");
		}
	}
	for (const CylinderRecord &record : records) {
		std::set<std::string> visited;
		const CylinderRecord *current = &record;
		while (current != nullptr &&
		       !current->parentCylinderIdentifier().empty()) {
			if (!visited.insert(current->cylinderIdentifier()).second) {
				rejected.insert(record.cylinderIdentifier());
				add_issue(
					issues, IssueCode::DisconnectedGraph,
					record.cylinderIdentifier(),
					"QSM cylinder graph contains a parent cycle.");
				break;
			}
			const auto parent = records_by_identifier.find(
				current->parentCylinderIdentifier());
			if (parent == records_by_identifier.end()) break;
			current = parent->second;
		}
	}
	return rejected;
}

}

VegetationMeasuredSourceAdapterReport
VegetationQuantitativeStructureModelAdapter::adapt(
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
			"QSM payload schema is unsupported.");
	}
	const std::optional<PlantArchitecture> architecture =
		reader.requiredArchitecture(
			*document, "plant_architecture", "plant_architecture");
	if (architecture.has_value() && *architecture != subject_scope.architecture()) {
		add_issue(
			issues, IssueCode::ArchitectureMismatch, std::string(),
			"QSM plant architecture does not match the calibration subject.");
	}
	const std::string graph_identifier = reader.requiredString(
		*document, "graph_identifier", "graph_identifier");
	const std::string length_unit = reader.requiredString(
		*document, "length_unit", "length_unit");
	const Json::Value *cylinders = reader.requiredArray(
		*document, "cylinders", "cylinders");

	std::vector<CylinderRecord> records;
	std::set<std::string> identifiers;
	std::set<std::string> parse_rejected_identifiers;
	if (cylinders != nullptr) {
		for (Json::ArrayIndex index = 0u; index < cylinders->size(); ++index) {
			const Json::Value &cylinder = (*cylinders)[index];
			const std::string path = "cylinders[" + std::to_string(index) + "]";
			const std::string identifier = reader.requiredString(
				cylinder, "identifier", path + ".identifier");
			const std::size_t issue_count_before = issues.size();
			const std::string parent_identifier = reader.optionalString(
				cylinder, "parent_identifier", path + ".parent_identifier",
				identifier);
			const std::optional<std::size_t> branch_order = reader.requiredCount(
				cylinder, "branch_order", path + ".branch_order", identifier);
			const std::optional<VegetationMeasuredPoint3d> start =
				reader.requiredPoint(
					cylinder, "start", path + ".start", length_unit,
					artifact.coordinateSystem(),
					identifier);
			const std::optional<VegetationMeasuredPoint3d> end =
				reader.requiredPoint(
					cylinder, "end", path + ".end", length_unit,
					artifact.coordinateSystem(),
					identifier);
			const std::optional<double> radius = reader.requiredDecimal(
				cylinder, "radius", path + ".radius", identifier);
			std::optional<double> radius_metres;
			if (radius.has_value()) {
				const auto normalized =
					VegetationMeasurementUnitNormalizationService().normalize(
						*radius, length_unit,
						VegetationMeasurementQuantityKind::LengthMetres);
				if (normalized.succeeded()) {
					radius_metres = normalized.normalizedValue();
				} else {
					add_issue(
						issues, IssueCode::UnsupportedUnit, identifier,
						"QSM cylinder radius unit is unsupported.");
				}
			}
			if (!identifier.empty() && !identifiers.insert(identifier).second) {
				add_issue(
					issues, IssueCode::DuplicateRecordIdentifier, identifier,
					"QSM cylinder identifier appears more than once.");
			}
			if (start.has_value() && end.has_value() &&
			    distance(*start, *end) <= 0.0) {
				add_issue(
					issues, IssueCode::InvalidValueRange, identifier,
					"QSM cylinder must have positive measured length.");
			}
			if (radius_metres.has_value() && *radius_metres <= 0.0) {
				add_issue(
					issues, IssueCode::InvalidValueRange, identifier,
					"QSM cylinder radius must be positive.");
			}
			if (issues.size() != issue_count_before || identifier.empty() ||
			    !branch_order.has_value() || !start.has_value() ||
			    !end.has_value() || !radius_metres.has_value()) {
				if (!identifier.empty()) parse_rejected_identifiers.insert(identifier);
				observations.emplace_back(
					identifier.empty() ? path : identifier,
					Disposition::Rejected,
					"QSM cylinder was rejected while reading source fields.");
				continue;
			}
			records.emplace_back(
				identifier, parent_identifier, *branch_order, *start, *end,
				*radius_metres);
		}
	}
	if (records.empty()) {
		add_issue(
			issues, IssueCode::IncompleteGraph, std::string(),
			"QSM payload requires at least one valid cylinder.");
	}
	std::set<std::string> rejected_identifiers = parse_rejected_identifiers;
	const std::set<std::string> graph_rejections =
		graph_rejected_identifiers(records, issues);
	rejected_identifiers.insert(
		graph_rejections.begin(), graph_rejections.end());

	std::size_t maximum_branch_order = 0u;
	for (const CylinderRecord &record : records) {
		maximum_branch_order =
			std::max(maximum_branch_order, record.branchOrder());
		observations.emplace_back(
			record.cylinderIdentifier(),
			rejected_identifiers.count(record.cylinderIdentifier()) == 0u
				? Disposition::Accepted
				: Disposition::Rejected,
			rejected_identifiers.count(record.cylinderIdentifier()) == 0u
				? "QSM cylinder contributed to the validated topology graph."
				: "QSM cylinder was rejected by topology graph validation.");
	}
	if (maximum_branch_order == 0u) {
		add_issue(
			issues, IssueCode::IncompleteGraph, graph_identifier,
			"QSM graph requires at least one measured branch above order zero.");
	}
	if (!issues.empty()) {
		return VegetationMeasuredSourceAdapterReport(
			std::move(issues), std::move(observations), std::nullopt);
	}

	const VegetationMeasuredCalibrationMeasurementFactory measurements;
	std::vector<VegetationCalibrationMeasurement> output = {
		measurements.createText(
			MeasurementKind::ShootTopologyArtifactIdentifier,
			graph_identifier,
			"identifier",
			subject_scope,
			artifact),
		measurements.createCount(
			MeasurementKind::MaximumBranchOrder,
			maximum_branch_order,
			subject_scope,
			artifact),
	};
	return VegetationMeasuredSourceAdapterReport(
		std::move(issues),
		std::move(observations),
		VegetationMeasuredEvidenceBundleFactory().create(
			subject_scope, artifact, "qsm-v1", std::move(output)));
}
