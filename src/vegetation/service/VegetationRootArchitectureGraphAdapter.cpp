#include "vegetation/service/VegetationRootArchitectureGraphAdapter.h"

#include "VegetationMeasuredSourceJsonDocumentReader.h"
#include "vegetation/model/VegetationRootArchitectureGraphRecord.h"
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
using SegmentRecord = VegetationRootArchitectureGraphSegmentRecord;

constexpr const char *media_type =
	"application/vnd.progen3d.root-architecture+json";
constexpr const char *payload_schema =
	"ProGen3D-RootArchitectureGraph-v1";

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
	const std::vector<SegmentRecord> &records,
	std::vector<Issue> &issues)
{
	std::set<std::string> rejected;
	std::map<std::string, const SegmentRecord *> records_by_identifier;
	std::size_t root_count = 0u;
	for (const SegmentRecord &record : records) {
		records_by_identifier.emplace(record.segmentIdentifier(), &record);
		if (record.parentSegmentIdentifier().empty()) ++root_count;
	}
	if (root_count != 1u) {
		add_issue(
			issues, IssueCode::IncompleteGraph, std::string(),
			"Root architecture graph requires exactly one root segment.");
	}
	for (const SegmentRecord &record : records) {
		if (record.parentSegmentIdentifier().empty()) continue;
		const auto parent = records_by_identifier.find(
			record.parentSegmentIdentifier());
		if (parent == records_by_identifier.end()) {
			rejected.insert(record.segmentIdentifier());
			add_issue(
				issues, IssueCode::InvalidParentReference,
				record.segmentIdentifier(),
				"Root segment parent identifier does not exist.");
			continue;
		}
		if (record.rootOrder() < parent->second->rootOrder() ||
		    record.rootOrder() > parent->second->rootOrder() + 1u) {
			rejected.insert(record.segmentIdentifier());
			add_issue(
				issues, IssueCode::InvalidValueRange,
				record.segmentIdentifier(),
				"Root order must equal or increment its parent order by one.");
		}
	}
	for (const SegmentRecord &record : records) {
		std::set<std::string> visited;
		const SegmentRecord *current = &record;
		while (current != nullptr && !current->parentSegmentIdentifier().empty()) {
			if (!visited.insert(current->segmentIdentifier()).second) {
				rejected.insert(record.segmentIdentifier());
				add_issue(
					issues, IssueCode::DisconnectedGraph,
					record.segmentIdentifier(),
					"Root architecture graph contains a parent cycle.");
				break;
			}
			const auto parent = records_by_identifier.find(
				current->parentSegmentIdentifier());
			if (parent == records_by_identifier.end()) break;
			current = parent->second;
		}
	}
	return rejected;
}

}

VegetationMeasuredSourceAdapterReport
VegetationRootArchitectureGraphAdapter::adapt(
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
			"Root architecture payload schema is unsupported.");
	}
	const std::optional<PlantArchitecture> architecture =
		reader.requiredArchitecture(
			*document, "plant_architecture", "plant_architecture");
	if (architecture.has_value() && *architecture != subject_scope.architecture()) {
		add_issue(
			issues, IssueCode::ArchitectureMismatch, std::string(),
			"Root architecture does not match the calibration subject.");
	}
	const std::string graph_identifier = reader.requiredString(
		*document, "graph_identifier", "graph_identifier");
	const std::string length_unit = reader.requiredString(
		*document, "length_unit", "length_unit");
	const std::optional<VegetationMeasuredPoint3d> origin = reader.requiredPoint(
		*document, "root_origin", "root_origin", length_unit,
		artifact.coordinateSystem());
	const Json::Value *segments = reader.requiredArray(
		*document, "segments", "segments");

	std::vector<SegmentRecord> records;
	std::set<std::string> identifiers;
	std::set<std::string> parse_rejected_identifiers;
	if (segments != nullptr) {
		for (Json::ArrayIndex index = 0u; index < segments->size(); ++index) {
			const Json::Value &segment = (*segments)[index];
			const std::string path = "segments[" + std::to_string(index) + "]";
			const std::string identifier = reader.requiredString(
				segment, "identifier", path + ".identifier");
			const std::size_t issue_count_before = issues.size();
			const std::string parent_identifier = reader.optionalString(
				segment, "parent_identifier", path + ".parent_identifier",
				identifier);
			const std::optional<std::size_t> root_order = reader.requiredCount(
				segment, "root_order", path + ".root_order", identifier);
			const std::optional<VegetationMeasuredPoint3d> start =
				reader.requiredPoint(
					segment, "start", path + ".start", length_unit,
					artifact.coordinateSystem(),
					identifier);
			const std::optional<VegetationMeasuredPoint3d> end =
				reader.requiredPoint(
					segment, "end", path + ".end", length_unit,
					artifact.coordinateSystem(),
					identifier);
			const std::optional<double> radius = reader.requiredDecimal(
				segment, "radius", path + ".radius", identifier);
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
						"Root segment radius unit is unsupported.");
				}
			}
			if (!identifier.empty() && !identifiers.insert(identifier).second) {
				add_issue(
					issues, IssueCode::DuplicateRecordIdentifier, identifier,
					"Root segment identifier appears more than once.");
			}
			if (start.has_value() && end.has_value() &&
			    distance(*start, *end) <= 0.0) {
				add_issue(
					issues, IssueCode::InvalidValueRange, identifier,
					"Root segment must have positive measured length.");
			}
			if (radius_metres.has_value() && *radius_metres <= 0.0) {
				add_issue(
					issues, IssueCode::InvalidValueRange, identifier,
					"Root segment radius must be positive.");
			}
			if (issues.size() != issue_count_before || identifier.empty() ||
			    !root_order.has_value() || !start.has_value() ||
			    !end.has_value() || !radius_metres.has_value()) {
				if (!identifier.empty()) parse_rejected_identifiers.insert(identifier);
				observations.emplace_back(
					identifier.empty() ? path : identifier,
					Disposition::Rejected,
					"Root segment was rejected while reading source fields.");
				continue;
			}
			records.emplace_back(
				identifier, parent_identifier, *root_order, *start, *end,
				*radius_metres);
		}
	}
	if (records.empty()) {
		add_issue(
			issues, IssueCode::IncompleteGraph, std::string(),
			"Root architecture payload requires at least one valid segment.");
	}
	std::set<std::string> rejected_identifiers = parse_rejected_identifiers;
	const std::set<std::string> graph_rejections =
		graph_rejected_identifiers(records, issues);
	rejected_identifiers.insert(
		graph_rejections.begin(), graph_rejections.end());

	std::size_t maximum_root_order = 0u;
	double maximum_depth_metres = 0.0;
	double maximum_radial_spread_metres = 0.0;
	if (origin.has_value()) {
		for (const SegmentRecord &record : records) {
			maximum_root_order = std::max(maximum_root_order, record.rootOrder());
			for (const VegetationMeasuredPoint3d *point :
			     {&record.startPointMetres(), &record.endPointMetres()}) {
				maximum_depth_metres = std::max(
					maximum_depth_metres, origin->z() - point->z());
				const double x = point->x() - origin->x();
				const double y = point->y() - origin->y();
				maximum_radial_spread_metres = std::max(
					maximum_radial_spread_metres, std::sqrt(x * x + y * y));
			}
		}
	}
	for (const SegmentRecord &record : records) {
		observations.emplace_back(
			record.segmentIdentifier(),
			rejected_identifiers.count(record.segmentIdentifier()) == 0u
				? Disposition::Accepted
				: Disposition::Rejected,
			rejected_identifiers.count(record.segmentIdentifier()) == 0u
				? "Root segment contributed to the validated root graph."
				: "Root segment was rejected by graph validation.");
	}
	if (maximum_root_order == 0u || maximum_depth_metres <= 0.0 ||
	    maximum_radial_spread_metres <= 0.0) {
		add_issue(
			issues, IssueCode::IncompleteGraph, graph_identifier,
			"Root graph must establish positive depth, radial spread, and root order.");
	}
	if (!issues.empty()) {
		return VegetationMeasuredSourceAdapterReport(
			std::move(issues), std::move(observations), std::nullopt);
	}

	const VegetationMeasuredCalibrationMeasurementFactory measurements;
	std::vector<VegetationCalibrationMeasurement> output = {
		measurements.createText(
			MeasurementKind::RootGraphIdentifier,
			graph_identifier,
			"identifier",
			subject_scope,
			artifact),
		measurements.createDecimal(
			MeasurementKind::MaximumRootDepthMetres,
			maximum_depth_metres,
			"m",
			subject_scope,
			artifact),
		measurements.createDecimal(
			MeasurementKind::MaximumRootRadialSpreadMetres,
			maximum_radial_spread_metres,
			"m",
			subject_scope,
			artifact),
		measurements.createCount(
			MeasurementKind::MaximumRootOrder,
			maximum_root_order,
			subject_scope,
			artifact),
	};
	return VegetationMeasuredSourceAdapterReport(
		std::move(issues),
		std::move(observations),
		VegetationMeasuredEvidenceBundleFactory().create(
			subject_scope, artifact, "root-graph-v1", std::move(output)));
}
