#include "vegetation/service/VegetationTreeQsmCylinderTableDecoder.h"

#include "VegetationDelimitedSourceTableReader.h"
#include "vegetation/model/PlantArchitecture.h"
#include "vegetation/model/VegetationMeasuredPoint3d.h"
#include "vegetation/service/VegetationDecodedMeasuredSourceArtifactFactory.h"
#include "vegetation/service/VegetationMeasuredPointCanonicalizationService.h"
#include "vegetation/service/VegetationMeasuredSourceDecoderArtifactValidationService.h"
#include "vegetation/service/VegetationMeasurementUnitNormalizationService.h"

#include <json/json.h>

#include <algorithm>
#include <array>
#include <cmath>
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
	"VegetationTreeQsmCylinderTableDecoder";
constexpr const char *source_media_type = "text/vnd.treeqsm.cylinder-table";
constexpr const char *source_schema_version = "TreeQSM-save_model_text-1.1.0";
constexpr const char *canonical_media_type =
	"application/vnd.progen3d.qsm+json";
constexpr const char *canonical_schema_version =
	"ProGen3D-QuantitativeStructureModel-v1";

const std::vector<std::string> official_header = {
	"radius (m)",
	"length (m)",
	"start_point",
	"axis_direction",
	"parent",
	"extension",
	"branch",
	"branch_order",
	"position_in_branch",
	"mad",
	"SurfCov",
	"added",
	"UnmodRadius (m)",
};

struct TreeQsmCylinderSourceRecord
{
	std::string identifier;
	std::size_t parent_index = 0u;
	std::size_t branch_order = 0u;
	VegetationMeasuredPoint3d start_point;
	VegetationMeasuredPoint3d end_point;
	double radius_metres = 0.0;
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

double vector_length(double x, double y, double z)
{
	return std::sqrt(x * x + y * y + z * z);
}

std::optional<double> uniform_transform_scale(
	const VegetationMeasuredSourceDecodeContext &context)
{
	if (!context.coordinateReferenceTransform().has_value()) return 1.0;
	const auto &matrix =
		context.coordinateReferenceTransform()->sourceMetresToTargetMetres();
	const std::array<double, 3> scales = {
		vector_length(matrix[0], matrix[4], matrix[8]),
		vector_length(matrix[1], matrix[5], matrix[9]),
		vector_length(matrix[2], matrix[6], matrix[10]),
	};
	if (scales[0] <= 0.0 ||
	    std::abs(scales[0] - scales[1]) > 1.0e-9 ||
	    std::abs(scales[0] - scales[2]) > 1.0e-9) {
		return std::nullopt;
	}
	const double dot_xy =
		matrix[0] * matrix[1] + matrix[4] * matrix[5] +
		matrix[8] * matrix[9];
	const double dot_xz =
		matrix[0] * matrix[2] + matrix[4] * matrix[6] +
		matrix[8] * matrix[10];
	const double dot_yz =
		matrix[1] * matrix[2] + matrix[5] * matrix[6] +
		matrix[9] * matrix[10];
	if (std::abs(dot_xy) > 1.0e-9 || std::abs(dot_xz) > 1.0e-9 ||
	    std::abs(dot_yz) > 1.0e-9) {
		return std::nullopt;
	}
	return scales[0];
}

Json::Value point_json(const VegetationMeasuredPoint3d &point)
{
	Json::Value value(Json::arrayValue);
	value.append(point.x());
	value.append(point.y());
	value.append(point.z());
	return value;
}

}

VegetationMeasuredSourceDecodeReport
VegetationTreeQsmCylinderTableDecoder::decode(
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
	const auto table = reader.read(artifact.sourcePayload(), '\t', issues);
	if (!table.has_value()) {
		return report(
			artifact, std::move(issues), std::move(observations), std::nullopt);
	}
	if (table->columnNames() != official_header) {
		issues.emplace_back(
			IssueCode::UnexpectedColumnLayout, "header",
			"TreeQSM cylinder table header does not match save_model_text 1.1.0.");
	}
	std::vector<std::string> numeric_tokens;
	for (const auto &physical_line : table->records()) {
		numeric_tokens.insert(
			numeric_tokens.end(), physical_line.begin(), physical_line.end());
	}
	if (numeric_tokens.empty()) {
		issues.emplace_back(
			IssueCode::IncompleteSourceTopology, std::string(),
			"TreeQSM cylinder table requires at least one cylinder record.");
	} else if (numeric_tokens.size() % 17u != 0u) {
		issues.emplace_back(
			IssueCode::UnexpectedColumnLayout, std::string(),
			"TreeQSM cylinder token stream must contain a multiple of seventeen values.");
	}
	const std::optional<double> transform_scale = uniform_transform_scale(context);
	if (!transform_scale.has_value()) {
		issues.emplace_back(
			IssueCode::InvalidCoordinateReferenceTransform, std::string(),
			"TreeQSM cylinder radii require a rigid or uniformly scaled affine transform.");
	}
	if (!issues.empty()) {
		return report(
			artifact, std::move(issues), std::move(observations), std::nullopt);
	}

	std::vector<TreeQsmCylinderSourceRecord> cylinders;
	const std::size_t logical_record_count = numeric_tokens.size() / 17u;
	cylinders.reserve(logical_record_count);
	for (std::size_t row_index = 0u; row_index < logical_record_count;
	     ++row_index) {
		const auto row_begin =
			numeric_tokens.begin() + static_cast<std::ptrdiff_t>(row_index * 17u);
		const std::vector<std::string> row(row_begin, row_begin + 17);
		const std::string identifier =
			"cylinder-" + std::to_string(row_index + 1u);
		const std::size_t issue_count_before = issues.size();
		std::array<std::optional<double>, 17> values;
		for (std::size_t column = 0u; column < row.size(); ++column) {
			values[column] = reader.readDecimal(
				row[column], identifier,
				"column-" + std::to_string(column + 1u), issues);
		}
		const auto parent = reader.readInteger(
			row[8], identifier, "parent", issues);
		const auto branch_order = reader.readInteger(
			row[11], identifier, "branch_order", issues);
		if (issues.size() != issue_count_before ||
		    std::any_of(values.begin(), values.end(), [](const auto &value) {
			    return !value.has_value();
		    }) ||
		    !parent.has_value() || !branch_order.has_value()) {
			observations.emplace_back(
				identifier, ObservationKind::RejectedRecord,
				"Cylinder row was rejected while parsing numeric fields.");
			continue;
		}
		if (*values[0] <= 0.0 || *values[1] <= 0.0 ||
		    *parent < 0 || *branch_order < 0) {
			issues.emplace_back(
				IssueCode::InvalidValueRange, identifier,
				"TreeQSM radius and length must be positive; parent and branch order must be non-negative.");
			observations.emplace_back(
				identifier, ObservationKind::RejectedRecord,
				"Cylinder row was rejected because a value is outside its valid range.");
			continue;
		}
		const VegetationMeasuredPoint3d source_start(
			*values[2], *values[3], *values[4]);
		const double axis_length =
			vector_length(*values[5], *values[6], *values[7]);
		if (axis_length <= 0.0) {
			issues.emplace_back(
				IssueCode::InvalidValueRange, identifier,
				"TreeQSM axis direction must have positive length.");
			observations.emplace_back(
				identifier, ObservationKind::RejectedRecord,
				"Cylinder row was rejected because its axis is degenerate.");
			continue;
		}
		const VegetationMeasuredPoint3d source_end(
			source_start.x() + *values[1] * *values[5],
			source_start.y() + *values[1] * *values[6],
			source_start.z() + *values[1] * *values[7]);
		const auto canonical_start =
			VegetationMeasuredPointCanonicalizationService()
				.mapToCanonicalPlantSpace(
					source_start, artifact.coordinateSystem(), "m",
					context.coordinateReferenceTransform());
		const auto canonical_end =
			VegetationMeasuredPointCanonicalizationService()
				.mapToCanonicalPlantSpace(
					source_end, artifact.coordinateSystem(), "m",
					context.coordinateReferenceTransform());
		if (!canonical_start.succeeded() || !canonical_end.succeeded()) {
			issues.emplace_back(
				IssueCode::InvalidCoordinateReferenceTransform, identifier,
				"TreeQSM cylinder endpoints cannot be mapped to canonical plant space.");
			observations.emplace_back(
				identifier, ObservationKind::RejectedRecord,
				"Cylinder row was rejected during coordinate mapping.");
			continue;
		}
		cylinders.push_back(TreeQsmCylinderSourceRecord{
			identifier,
			static_cast<std::size_t>(*parent),
			static_cast<std::size_t>(*branch_order),
			*canonical_start.transformedPoint(),
			*canonical_end.transformedPoint(),
			*values[0] * *transform_scale,
		});
		observations.emplace_back(
			identifier, ObservationKind::DecodedRecord,
			"TreeQSM cylinder row was decoded into canonical start, end, radius, parent, and branch-order fields.");
		observations.emplace_back(
			identifier, ObservationKind::DerivedGeometry,
			"Canonical cylinder end point was derived from exported start point, axis direction, and length.");
	}

	std::size_t root_count = 0u;
	for (std::size_t index = 0u; index < cylinders.size(); ++index) {
		const auto &cylinder = cylinders[index];
		if (cylinder.parent_index == 0u) {
			++root_count;
		} else if (cylinder.parent_index > index) {
			issues.emplace_back(
				IssueCode::InvalidParentReference, cylinder.identifier,
				"TreeQSM parent index must identify an earlier cylinder row.");
		}
	}
	if (root_count != 1u || cylinders.size() != logical_record_count) {
		issues.emplace_back(
			IssueCode::IncompleteSourceTopology, std::string(),
			"TreeQSM cylinder graph requires exactly one root and every row must decode.");
	}
	observations.emplace_back(
		"TreeQSM-auxiliary-cylinder-fields", ObservationKind::DeferredField,
		"Extension, branch index, position-in-branch, MAD, surface coverage, added-cylinder flag, and unmodified radius remain preserved only in the hashed raw source.");
	if (!issues.empty()) {
		return report(
			artifact, std::move(issues), std::move(observations), std::nullopt);
	}

	Json::Value document(Json::objectValue);
	document["schema_version"] = canonical_schema_version;
	document["plant_architecture"] =
		plantArchitectureName(context.plantArchitecture());
	document["graph_identifier"] = context.canonicalRecordIdentifier();
	document["length_unit"] = "m";
	Json::Value cylinder_array(Json::arrayValue);
	for (const auto &cylinder : cylinders) {
		Json::Value value(Json::objectValue);
		value["identifier"] = cylinder.identifier;
		value["parent_identifier"] =
			cylinder.parent_index == 0u
				? std::string()
				: cylinders[cylinder.parent_index - 1u].identifier;
		value["branch_order"] =
			static_cast<Json::UInt64>(cylinder.branch_order);
		value["start"] = point_json(cylinder.start_point);
		value["end"] = point_json(cylinder.end_point);
		value["radius"] = cylinder.radius_metres;
		cylinder_array.append(std::move(value));
	}
	document["cylinders"] = std::move(cylinder_array);
	const VegetationMeasuredSourceArtifact canonical_artifact =
		VegetationDecodedMeasuredSourceArtifactFactory().create(
			artifact, context, decoder_identifier, canonical_media_type,
			std::move(document));
	return report(
		artifact, std::move(issues), std::move(observations),
		canonical_artifact);
}
