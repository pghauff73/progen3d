#include "vegetation/service/VegetationRootSystemMarkupLanguageDecoder.h"

#include "vegetation/model/PlantArchitecture.h"
#include "vegetation/model/VegetationMeasuredPoint3d.h"
#include "vegetation/service/VegetationDecodedMeasuredSourceArtifactFactory.h"
#include "vegetation/service/VegetationMeasuredPointCanonicalizationService.h"
#include "vegetation/service/VegetationMeasuredSourceDecoderArtifactValidationService.h"
#include "vegetation/service/VegetationMeasurementUnitNormalizationService.h"

#include <json/json.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

using Issue = VegetationMeasuredSourceDecodeIssue;
using IssueCode = VegetationMeasuredSourceDecodeIssueCode;
using Observation = VegetationMeasuredSourceDecodeObservation;
using ObservationKind = VegetationMeasuredSourceDecodeObservationKind;

constexpr const char *decoder_identifier =
	"VegetationRootSystemMarkupLanguageDecoder";
constexpr const char *source_media_type = "application/rsml+xml";
constexpr const char *source_schema_version = "RSML-1";
constexpr const char *canonical_media_type =
	"application/vnd.progen3d.root-architecture+json";
constexpr const char *canonical_schema_version =
	"ProGen3D-RootArchitectureGraph-v1";

struct RootArchitectureSegment
{
	std::string identifier;
	std::string parent_identifier;
	std::size_t root_order = 0u;
	VegetationMeasuredPoint3d start_point;
	VegetationMeasuredPoint3d end_point;
	double radius_metres = 0.0;
};

bool element_named(const xmlNode *node, const char *name)
{
	return node != nullptr && node->type == XML_ELEMENT_NODE &&
	       xmlStrEqual(node->name, BAD_CAST name) != 0;
}

xmlNode *first_child(xmlNode *parent, const char *name)
{
	if (parent == nullptr) return nullptr;
	for (xmlNode *child = parent->children; child != nullptr;
	     child = child->next) {
		if (element_named(child, name)) return child;
	}
	return nullptr;
}

std::vector<xmlNode *> children_named(xmlNode *parent, const char *name)
{
	std::vector<xmlNode *> children;
	if (parent == nullptr) return children;
	for (xmlNode *child = parent->children; child != nullptr;
	     child = child->next) {
		if (element_named(child, name)) children.push_back(child);
	}
	return children;
}

std::string node_text(xmlNode *node)
{
	if (node == nullptr) return std::string();
	xmlChar *content = xmlNodeGetContent(node);
	if (content == nullptr) return std::string();
	const std::string value(reinterpret_cast<const char *>(content));
	xmlFree(content);
	return value;
}

std::string attribute(xmlNode *node, const char *name)
{
	if (node == nullptr) return std::string();
	xmlChar *value = xmlGetProp(node, BAD_CAST name);
	if (value == nullptr) return std::string();
	const std::string result(reinterpret_cast<const char *>(value));
	xmlFree(value);
	return result;
}

std::optional<double> decimal_value(const std::string &text)
{
	errno = 0;
	char *end = nullptr;
	const double value = std::strtod(text.c_str(), &end);
	if (end == text.c_str() || *end != '\0' || errno == ERANGE ||
	    !std::isfinite(value)) {
		return std::nullopt;
	}
	return value;
}

std::optional<std::string> canonical_length_unit(const std::string &unit)
{
	if (unit == "m" || unit == "metre") return "m";
	if (unit == "cm" || unit == "centimetre") return "cm";
	if (unit == "mm" || unit == "millimetre") return "mm";
	if (unit == "um" || unit == "micrometre") return "um";
	if (unit == "nm" || unit == "nanometre") return "nm";
	return std::nullopt;
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

double squared_distance_to_segment(
	const VegetationMeasuredPoint3d &point,
	const RootArchitectureSegment &segment)
{
	const double axis_x = segment.end_point.x() - segment.start_point.x();
	const double axis_y = segment.end_point.y() - segment.start_point.y();
	const double axis_z = segment.end_point.z() - segment.start_point.z();
	const double offset_x = point.x() - segment.start_point.x();
	const double offset_y = point.y() - segment.start_point.y();
	const double offset_z = point.z() - segment.start_point.z();
	const double denominator =
		axis_x * axis_x + axis_y * axis_y + axis_z * axis_z;
	const double parameter = denominator <= 0.0
		? 0.0
		: std::clamp(
			  (offset_x * axis_x + offset_y * axis_y + offset_z * axis_z) /
				  denominator,
			  0.0,
			  1.0);
	const double closest_x = segment.start_point.x() + parameter * axis_x;
	const double closest_y = segment.start_point.y() + parameter * axis_y;
	const double closest_z = segment.start_point.z() + parameter * axis_z;
	const double difference_x = point.x() - closest_x;
	const double difference_y = point.y() - closest_y;
	const double difference_z = point.z() - closest_z;
	return difference_x * difference_x + difference_y * difference_y +
	       difference_z * difference_z;
}

Json::Value point_json(const VegetationMeasuredPoint3d &point)
{
	Json::Value value(Json::arrayValue);
	value.append(point.x());
	value.append(point.y());
	value.append(point.z());
	return value;
}

VegetationMeasuredSourceDecodeReport make_report(
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

class RootSystemMarkupLanguageDocumentDecoder
{
public:
	RootSystemMarkupLanguageDocumentDecoder(
		const VegetationMeasuredSourceArtifact &artifact,
		const VegetationMeasuredSourceDecodeContext &context,
		std::vector<Issue> &issues,
		std::vector<Observation> &observations)
		: artifact_(artifact),
		  context_(context),
		  issues_(issues),
		  observations_(observations)
	{
	}

	std::optional<Json::Value> decode();

private:
	void readMetadata(xmlNode *metadata);
	std::optional<VegetationMeasuredPoint3d> readPoint(
		xmlNode *point,
		const std::string &record_identifier);
	std::optional<std::vector<double>> readDiameters(
		xmlNode *root,
		std::size_t point_count,
		const std::string &root_identifier);
	std::optional<double> canonicalRadius(
		double source_diameter,
		const std::string &record_identifier);
	std::string nearestParentIdentifier(
		const VegetationMeasuredPoint3d &child_origin,
		const std::vector<std::size_t> &parent_segment_indices) const;
	void decodeRoot(
		xmlNode *root,
		std::size_t root_order,
		const std::vector<std::size_t> &parent_segment_indices);

	const VegetationMeasuredSourceArtifact &artifact_;
	const VegetationMeasuredSourceDecodeContext &context_;
	std::vector<Issue> &issues_;
	std::vector<Observation> &observations_;
	std::optional<std::string> length_unit_;
	double resolution_ = 0.0;
	std::optional<double> transform_scale_;
	std::optional<VegetationMeasuredPoint3d> root_origin_;
	std::set<std::string> root_identifiers_;
	std::size_t derived_root_count_ = 0u;
	std::vector<RootArchitectureSegment> segments_;
};

std::optional<Json::Value> RootSystemMarkupLanguageDocumentDecoder::decode()
{
	using XmlDocument = std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)>;
	XmlDocument document(
		xmlReadMemory(
			artifact_.sourcePayload().data(),
			static_cast<int>(artifact_.sourcePayload().size()),
			artifact_.sourceLocator().c_str(),
			nullptr,
			XML_PARSE_NONET | XML_PARSE_NOBLANKS | XML_PARSE_NOERROR |
				XML_PARSE_NOWARNING),
		xmlFreeDoc);
	if (!document) {
		issues_.emplace_back(
			IssueCode::MalformedSourcePayload, std::string(),
			"RSML source is not well-formed XML.");
		return std::nullopt;
	}
	xmlNode *rsml = xmlDocGetRootElement(document.get());
	if (!element_named(rsml, "rsml")) {
		issues_.emplace_back(
			IssueCode::MalformedSourcePayload, std::string(),
			"RSML document root must be rsml.");
		return std::nullopt;
	}
	xmlNode *metadata = first_child(rsml, "metadata");
	xmlNode *scene = first_child(rsml, "scene");
	if (metadata == nullptr || scene == nullptr) {
		issues_.emplace_back(
			IssueCode::MissingRequiredField, std::string(),
			"RSML document requires metadata and scene elements.");
		return std::nullopt;
	}
	readMetadata(metadata);
	const auto plants = children_named(scene, "plant");
	if (plants.size() != 1u) {
		issues_.emplace_back(
			IssueCode::IncompleteSourceTopology, "scene",
			"One canonical root graph must decode from exactly one RSML plant.");
		return std::nullopt;
	}
	if (first_child(scene, "properties") != nullptr ||
	    first_child(scene, "annotations") != nullptr) {
		observations_.emplace_back(
			"scene-metadata", ObservationKind::DeferredField,
			"RSML scene properties and annotations remain preserved only in the hashed raw source.");
	}
	xmlNode *plant = plants.front();
	if (first_child(plant, "properties") != nullptr ||
	    first_child(plant, "annotations") != nullptr) {
		observations_.emplace_back(
			attribute(plant, "id"), ObservationKind::DeferredField,
			"RSML plant properties and annotations remain preserved only in the hashed raw source.");
	}
	const auto top_roots = children_named(plant, "root");
	if (top_roots.size() != 1u) {
		issues_.emplace_back(
			IssueCode::IncompleteSourceTopology, attribute(plant, "id"),
			"Canonical root architecture requires exactly one top-level RSML root axis.");
		return std::nullopt;
	}
	if (!issues_.empty()) return std::nullopt;
	decodeRoot(top_roots.front(), 0u, {});
	if (!issues_.empty() || segments_.empty() || !root_origin_.has_value()) {
		return std::nullopt;
	}

	Json::Value canonical(Json::objectValue);
	canonical["schema_version"] = canonical_schema_version;
	canonical["plant_architecture"] =
		plantArchitectureName(context_.plantArchitecture());
	canonical["graph_identifier"] = context_.canonicalRecordIdentifier();
	canonical["length_unit"] = "m";
	canonical["root_origin"] = point_json(*root_origin_);
	Json::Value segment_array(Json::arrayValue);
	for (const RootArchitectureSegment &segment : segments_) {
		Json::Value value(Json::objectValue);
		value["identifier"] = segment.identifier;
		value["parent_identifier"] = segment.parent_identifier;
		value["root_order"] = static_cast<Json::UInt64>(segment.root_order);
		value["start"] = point_json(segment.start_point);
		value["end"] = point_json(segment.end_point);
		value["radius"] = segment.radius_metres;
		segment_array.append(std::move(value));
	}
	canonical["segments"] = std::move(segment_array);
	return canonical;
}

void RootSystemMarkupLanguageDocumentDecoder::readMetadata(xmlNode *metadata)
{
	const std::string version = node_text(first_child(metadata, "version"));
	if (version != "1" && version != "1.0") {
		issues_.emplace_back(
			IssueCode::UnsupportedSourceSchemaVersion, "metadata.version",
			"RSML metadata version must be 1 or 1.0.");
	}
	const std::string declared_unit = node_text(first_child(metadata, "unit"));
	length_unit_ = canonical_length_unit(declared_unit);
	if (!length_unit_.has_value()) {
		issues_.emplace_back(
			IssueCode::UnsupportedUnit, "metadata.unit",
			"RSML decoder requires a standard metric unit; pixel-only geometry requires external calibration.");
	}
	const auto resolution_value =
		decimal_value(node_text(first_child(metadata, "resolution")));
	if (!resolution_value.has_value() || *resolution_value <= 0.0) {
		issues_.emplace_back(
			IssueCode::InvalidValueRange, "metadata.resolution",
			"RSML resolution must be one positive finite pixel-to-unit conversion rate.");
	} else {
		resolution_ = *resolution_value;
	}
	const std::string file_key = node_text(first_child(metadata, "file-key"));
	if (file_key.empty()) {
		issues_.emplace_back(
			IssueCode::MissingRequiredField, "metadata.file-key",
			"RSML metadata requires a file-key for source identity.");
	}
	if (first_child(metadata, "property-definitions") != nullptr ||
	    first_child(metadata, "time-sequence") != nullptr ||
	    first_child(metadata, "image") != nullptr) {
		observations_.emplace_back(
			"metadata", ObservationKind::DeferredField,
			"RSML property definitions, time sequence, and image metadata remain preserved only in the hashed raw source.");
	}
	transform_scale_ = uniform_transform_scale(context_);
	if (!transform_scale_.has_value()) {
		issues_.emplace_back(
			IssueCode::InvalidCoordinateReferenceTransform, std::string(),
			"RSML segment radii require a rigid or uniformly scaled affine transform.");
	}
}

std::optional<VegetationMeasuredPoint3d>
RootSystemMarkupLanguageDocumentDecoder::readPoint(
	xmlNode *point,
	const std::string &record_identifier)
{
	const auto x = decimal_value(attribute(point, "x"));
	const auto y = decimal_value(attribute(point, "y"));
	const auto z = decimal_value(attribute(point, "z"));
	if (!x.has_value() || !y.has_value() || !z.has_value()) {
		issues_.emplace_back(
			IssueCode::MissingRequiredField, record_identifier,
			"Three-dimensional calibration requires x, y, and z on every RSML polyline point.");
		return std::nullopt;
	}
	if (!length_unit_.has_value()) return std::nullopt;
	const VegetationMeasuredPoint3d source_point(
		*x * resolution_, *y * resolution_, *z * resolution_);
	const auto canonical =
		VegetationMeasuredPointCanonicalizationService().mapToCanonicalPlantSpace(
			source_point,
			artifact_.coordinateSystem(),
			*length_unit_,
			context_.coordinateReferenceTransform());
	if (!canonical.succeeded()) {
		issues_.emplace_back(
			IssueCode::InvalidCoordinateReferenceTransform, record_identifier,
			"RSML point cannot be mapped to canonical plant space.");
		return std::nullopt;
	}
	return canonical.transformedPoint();
}

std::optional<std::vector<double>>
RootSystemMarkupLanguageDocumentDecoder::readDiameters(
	xmlNode *root,
	std::size_t point_count,
	const std::string &root_identifier)
{
	xmlNode *functions = first_child(root, "functions");
	if (functions == nullptr) {
		issues_.emplace_back(
			IssueCode::MissingRequiredField, root_identifier,
			"RSML root requires a polyline-domain diameter function for three-dimensional calibration.");
		return std::nullopt;
	}
	std::vector<double> diameter_samples;
	for (xmlNode *function : children_named(functions, "function")) {
		const std::string name = attribute(function, "name");
		if (name != "diameter") {
			observations_.emplace_back(
				root_identifier + ":function:" + name,
				ObservationKind::DeferredField,
				"Unsupported RSML function remains preserved only in the hashed raw source.");
			continue;
		}
		if (attribute(function, "domain") != "polyline") {
			issues_.emplace_back(
				IssueCode::MissingRequiredField, root_identifier,
				"RSML diameter function must use the polyline domain.");
			continue;
		}
		for (xmlNode *sample : children_named(function, "sample")) {
			const auto value = decimal_value(attribute(sample, "value"));
			if (!value.has_value() || *value <= 0.0) {
				issues_.emplace_back(
					IssueCode::InvalidValueRange, root_identifier,
					"RSML diameter samples must be positive finite values.");
				continue;
			}
			diameter_samples.push_back(*value);
		}
	}
	if (diameter_samples.size() != point_count &&
	    diameter_samples.size() + 1u != point_count) {
		issues_.emplace_back(
			IssueCode::IncompleteSourceTopology, root_identifier,
			"RSML diameter function must provide one sample per point or one sample per segment.");
		return std::nullopt;
	}
	return diameter_samples;
}

std::optional<double> RootSystemMarkupLanguageDocumentDecoder::canonicalRadius(
	double source_diameter,
	const std::string &record_identifier)
{
	if (!length_unit_.has_value() || !transform_scale_.has_value()) {
		return std::nullopt;
	}
	const auto diameter_metres =
		VegetationMeasurementUnitNormalizationService().normalize(
			source_diameter * resolution_,
			*length_unit_,
			VegetationMeasurementQuantityKind::LengthMetres);
	if (!diameter_metres.succeeded() ||
	    *diameter_metres.normalizedValue() <= 0.0) {
		issues_.emplace_back(
			IssueCode::UnsupportedUnit, record_identifier,
			"RSML diameter cannot be normalized to metres.");
		return std::nullopt;
	}
	return 0.5 * *diameter_metres.normalizedValue() * *transform_scale_;
}

std::string RootSystemMarkupLanguageDocumentDecoder::nearestParentIdentifier(
	const VegetationMeasuredPoint3d &child_origin,
	const std::vector<std::size_t> &parent_segment_indices) const
{
	double nearest_distance = std::numeric_limits<double>::infinity();
	std::string nearest_identifier;
	for (std::size_t index : parent_segment_indices) {
		const double distance =
			squared_distance_to_segment(child_origin, segments_[index]);
		if (distance < nearest_distance) {
			nearest_distance = distance;
			nearest_identifier = segments_[index].identifier;
		}
	}
	return nearest_identifier;
}

void RootSystemMarkupLanguageDocumentDecoder::decodeRoot(
	xmlNode *root,
	std::size_t root_order,
	const std::vector<std::size_t> &parent_segment_indices)
{
	std::string root_identifier = attribute(root, "id");
	if (root_identifier.empty()) {
		root_identifier = "derived-root-" + std::to_string(++derived_root_count_);
		observations_.emplace_back(
			root_identifier, ObservationKind::LossyConversion,
			"RSML root omitted its optional id; the decoder assigned a deterministic traversal identifier.");
	}
	if (!root_identifiers_.insert(root_identifier).second) {
		issues_.emplace_back(
			IssueCode::DuplicateRecordIdentifier, root_identifier,
			"RSML root identifier appears more than once.");
		return;
	}
	xmlNode *geometry = first_child(root, "geometry");
	xmlNode *polyline = first_child(geometry, "polyline");
	const auto point_nodes = children_named(polyline, "point");
	if (point_nodes.size() < 2u) {
		issues_.emplace_back(
			IssueCode::IncompleteSourceTopology, root_identifier,
			"RSML root polyline requires at least two points.");
		return;
	}
	std::vector<VegetationMeasuredPoint3d> points;
	for (std::size_t index = 0u; index < point_nodes.size(); ++index) {
		const auto point = readPoint(
			point_nodes[index],
			root_identifier + ":point-" + std::to_string(index));
		if (point.has_value()) points.push_back(*point);
	}
	const auto diameters =
		readDiameters(root, point_nodes.size(), root_identifier);
	if (points.size() != point_nodes.size() || !diameters.has_value()) {
		observations_.emplace_back(
			root_identifier, ObservationKind::RejectedRecord,
			"RSML root was rejected because its geometry or diameter evidence is incomplete.");
		return;
	}
	if (root_order == 0u) root_origin_ = points.front();

	std::vector<std::size_t> current_segment_indices;
	for (std::size_t index = 0u; index + 1u < points.size(); ++index) {
		const std::string segment_identifier =
			root_identifier + ":segment-" + std::to_string(index);
		double diameter = 0.0;
		if (diameters->size() == points.size()) {
			diameter = 0.5 * ((*diameters)[index] + (*diameters)[index + 1u]);
			observations_.emplace_back(
				segment_identifier, ObservationKind::LossyConversion,
				"Segment diameter is the arithmetic mean of its two RSML polyline-point diameter samples.");
		} else {
			diameter = (*diameters)[index];
		}
		const auto radius = canonicalRadius(diameter, segment_identifier);
		if (!radius.has_value() ||
		    vector_length(
			    points[index + 1u].x() - points[index].x(),
			    points[index + 1u].y() - points[index].y(),
			    points[index + 1u].z() - points[index].z()) <= 0.0) {
			issues_.emplace_back(
				IssueCode::InvalidValueRange, segment_identifier,
				"RSML segment must have positive length and radius.");
			observations_.emplace_back(
				segment_identifier, ObservationKind::RejectedRecord,
				"RSML segment was rejected because its derived geometry is degenerate.");
			continue;
		}
		std::string parent_identifier;
		if (index > 0u) {
			if (current_segment_indices.empty()) {
				issues_.emplace_back(
					IssueCode::InvalidParentReference, segment_identifier,
					"RSML segment cannot follow a rejected predecessor.");
				continue;
			}
			parent_identifier =
				segments_[current_segment_indices.back()].identifier;
		} else if (root_order > 0u) {
			parent_identifier =
				nearestParentIdentifier(points.front(), parent_segment_indices);
			if (parent_identifier.empty()) {
				issues_.emplace_back(
					IssueCode::InvalidParentReference, segment_identifier,
					"Nested RSML root cannot resolve a parent segment.");
				continue;
			}
			observations_.emplace_back(
				segment_identifier, ObservationKind::DerivedGeometry,
				"Nested root attachment was assigned to the nearest segment on its containing RSML root axis.");
		}
		segments_.push_back(RootArchitectureSegment{
			segment_identifier,
			parent_identifier,
			root_order,
			points[index],
			points[index + 1u],
			*radius,
		});
		current_segment_indices.push_back(segments_.size() - 1u);
		observations_.emplace_back(
			segment_identifier, ObservationKind::DecodedRecord,
			"RSML polyline interval was decoded into one canonical root segment.");
	}
	if (current_segment_indices.size() + 1u != points.size()) return;
	if (first_child(root, "properties") != nullptr ||
	    first_child(root, "annotations") != nullptr) {
		observations_.emplace_back(
			root_identifier, ObservationKind::DeferredField,
			"RSML root properties and annotations remain preserved only in the hashed raw source.");
	}
	if (geometry != nullptr) {
		for (xmlNode *child = geometry->children; child != nullptr;
		     child = child->next) {
			if (child->type == XML_ELEMENT_NODE &&
			    !element_named(child, "polyline")) {
				observations_.emplace_back(
					root_identifier + ":geometry:" +
						reinterpret_cast<const char *>(child->name),
					ObservationKind::DeferredField,
					"Non-polyline RSML geometry remains preserved only in the hashed raw source.");
			}
		}
	}
	for (xmlNode *child_root : children_named(root, "root")) {
		decodeRoot(child_root, root_order + 1u, current_segment_indices);
	}
}

}

VegetationMeasuredSourceDecodeReport
VegetationRootSystemMarkupLanguageDecoder::decode(
	const VegetationMeasuredSourceArtifact &artifact,
	const VegetationMeasuredSourceDecodeContext &context) const
{
	std::vector<Issue> issues =
		VegetationMeasuredSourceDecoderArtifactValidationService().validate(
			artifact, context, source_media_type, source_schema_version);
	std::vector<Observation> observations;
	if (!issues.empty()) {
		return make_report(
			artifact, std::move(issues), std::move(observations), std::nullopt);
	}
	RootSystemMarkupLanguageDocumentDecoder document_decoder(
		artifact, context, issues, observations);
	const auto canonical_document = document_decoder.decode();
	if (!canonical_document.has_value() || !issues.empty()) {
		return make_report(
			artifact, std::move(issues), std::move(observations), std::nullopt);
	}
	const VegetationMeasuredSourceArtifact canonical_artifact =
		VegetationDecodedMeasuredSourceArtifactFactory().create(
			artifact,
			context,
			decoder_identifier,
			canonical_media_type,
			*canonical_document);
	return make_report(
		artifact,
		std::move(issues),
		std::move(observations),
		canonical_artifact);
}
