#include "vegetation/service/VegetationPlyPointCloudDecoder.h"

#include "vegetation/service/VegetationMeasuredPointCanonicalizationService.h"
#include "vegetation/service/VegetationPointCloudSourceArtifactValidationService.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using Encoding = VegetationPointCloudEncoding;
using Issue = VegetationPointCloudIngestionIssue;
using IssueCode = VegetationPointCloudIngestionIssueCode;
using Observation = VegetationPointCloudIngestionObservation;
using ObservationKind = VegetationPointCloudIngestionObservationKind;
using OrganClass = VegetationPointCloudOrganClass;

constexpr const char *decoder_identifier = "VegetationPlyPointCloudDecoder";
constexpr const char *source_media_type = "application/ply";
constexpr const char *source_schema_version = "PLY-1.0";
constexpr const char *metadata_schema_version =
	"ProGen3D-VegetationPointCloudSourceMetadata-v1";

struct PlyScalarProperty
{
	std::string type;
	std::string name;
	std::size_t binary_size_bytes = 0u;
};

struct PlyHeader
{
	Encoding encoding = Encoding::Ascii;
	std::uint64_t vertex_count = 0u;
	std::vector<PlyScalarProperty> vertex_properties;
	std::size_t point_record_length_bytes = 0u;
	std::size_t point_data_offset = 0u;
};

VegetationPointCloudIngestionReport report(
	std::vector<Issue> issues,
	std::vector<Observation> observations,
	std::optional<VegetationPointCloudSourceMetadata> metadata,
	std::optional<VegetationPointCloudDataset> dataset)
{
	return VegetationPointCloudIngestionReport(
		decoder_identifier, std::move(issues), std::move(observations),
		std::move(metadata), std::move(dataset));
}

std::string without_carriage_return(std::string line)
{
	if (!line.empty() && line.back() == '\r') line.pop_back();
	return line;
}

std::vector<std::string> tokens(const std::string &line)
{
	std::istringstream stream(line);
	std::vector<std::string> values;
	std::string value;
	while (stream >> value) values.push_back(value);
	return values;
}

std::optional<std::size_t> scalar_size(const std::string &type)
{
	if (type == "char" || type == "int8" || type == "uchar" ||
	    type == "uint8") {
		return 1u;
	}
	if (type == "short" || type == "int16" || type == "ushort" ||
	    type == "uint16") {
		return 2u;
	}
	if (type == "int" || type == "int32" || type == "uint" ||
	    type == "uint32" || type == "float" || type == "float32") {
		return 4u;
	}
	if (type == "double" || type == "float64") return 8u;
	return std::nullopt;
}

std::optional<double> parse_number(const std::string &text)
{
	try {
		std::size_t consumed = 0u;
		const double value = std::stod(text, &consumed);
		if (consumed != text.size() || !std::isfinite(value)) return std::nullopt;
		return value;
	} catch (...) {
		return std::nullopt;
	}
}

std::optional<std::uint16_t> parse_uint16(const std::string &text)
{
	const auto number = parse_number(text);
	if (!number.has_value() || std::floor(*number) != *number || *number < 0.0 ||
	    *number > std::numeric_limits<std::uint16_t>::max()) {
		return std::nullopt;
	}
	return static_cast<std::uint16_t>(*number);
}

std::optional<std::uint8_t> parse_uint8(const std::string &text)
{
	const auto number = parse_uint16(text);
	if (!number.has_value() ||
	    *number > std::numeric_limits<std::uint8_t>::max()) {
		return std::nullopt;
	}
	return static_cast<std::uint8_t>(*number);
}

std::optional<std::uint64_t> read_binary_unsigned(
	const std::string &payload,
	std::size_t &offset,
	std::size_t byte_count,
	Encoding encoding)
{
	if (offset > payload.size() || byte_count > payload.size() - offset) {
		return std::nullopt;
	}
	std::uint64_t value = 0u;
	for (std::size_t byte_index = 0u; byte_index < byte_count; ++byte_index) {
		const std::size_t source_index =
			encoding == Encoding::BinaryLittleEndian
				? byte_index
				: byte_count - byte_index - 1u;
		value |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(
			         payload[offset + source_index]))
		         << (8u * byte_index);
	}
	offset += byte_count;
	return value;
}

std::optional<double> read_binary_scalar(
	const std::string &payload,
	std::size_t &offset,
	const std::string &type,
	Encoding encoding)
{
	const auto byte_count = scalar_size(type);
	if (!byte_count.has_value()) return std::nullopt;
	const auto bits =
		read_binary_unsigned(payload, offset, *byte_count, encoding);
	if (!bits.has_value()) return std::nullopt;
	if (type == "char" || type == "int8") {
		const std::int8_t value = static_cast<std::int8_t>(*bits);
		return static_cast<double>(value);
	}
	if (type == "uchar" || type == "uint8") {
		return static_cast<double>(static_cast<std::uint8_t>(*bits));
	}
	if (type == "short" || type == "int16") {
		const std::uint16_t unsigned_value = static_cast<std::uint16_t>(*bits);
		std::int16_t value = 0;
		std::memcpy(&value, &unsigned_value, sizeof(value));
		return static_cast<double>(value);
	}
	if (type == "ushort" || type == "uint16") {
		return static_cast<double>(static_cast<std::uint16_t>(*bits));
	}
	if (type == "int" || type == "int32") {
		const std::uint32_t unsigned_value = static_cast<std::uint32_t>(*bits);
		std::int32_t value = 0;
		std::memcpy(&value, &unsigned_value, sizeof(value));
		return static_cast<double>(value);
	}
	if (type == "uint" || type == "uint32") {
		return static_cast<double>(static_cast<std::uint32_t>(*bits));
	}
	if (type == "float" || type == "float32") {
		const std::uint32_t float_bits = static_cast<std::uint32_t>(*bits);
		float value = 0.0f;
		std::memcpy(&value, &float_bits, sizeof(value));
		if (!std::isfinite(value)) return std::nullopt;
		return static_cast<double>(value);
	}
	if (type == "double" || type == "float64") {
		const std::uint64_t double_bits = *bits;
		double value = 0.0;
		std::memcpy(&value, &double_bits, sizeof(value));
		if (!std::isfinite(value)) return std::nullopt;
		return value;
	}
	return std::nullopt;
}

std::string scalar_text(double value)
{
	std::ostringstream stream;
	stream << std::setprecision(std::numeric_limits<double>::max_digits10)
	       << value;
	return stream.str();
}

std::optional<PlyHeader> parse_header(
	const std::string &payload,
	std::vector<Issue> &issues)
{
	PlyHeader header;
	std::size_t line_start = 0u;
	std::size_t line_number = 0u;
	bool saw_format = false;
	bool saw_vertex = false;
	bool ended = false;
	std::string current_element;
	bool positive_element_before_vertex = false;

	while (line_start < payload.size()) {
		const std::size_t newline = payload.find('\n', line_start);
		const std::size_t line_end =
			newline == std::string::npos ? payload.size() : newline;
		const std::string line = without_carriage_return(
			payload.substr(line_start, line_end - line_start));
		++line_number;
		line_start = newline == std::string::npos ? payload.size() : newline + 1u;
		if (line_number == 1u) {
			if (line != "ply") {
				issues.emplace_back(
					IssueCode::MalformedHeader, "line-1",
					"PLY payload must begin with the exact magic word 'ply'.");
				return std::nullopt;
			}
			continue;
		}
		const auto values = tokens(line);
		if (line_number == 2u &&
		    (values.empty() || values[0] != "format")) {
			issues.emplace_back(
				IssueCode::MalformedHeader, "line-2",
				"PLY format declaration must be the second header line.");
			return std::nullopt;
		}
		if (values.empty() || values[0] == "comment" ||
		    values[0] == "obj_info") {
			continue;
		}
		if (values[0] == "format") {
			if (values.size() != 3u || values[2] != "1.0" || saw_format) {
				issues.emplace_back(
					IssueCode::MalformedHeader, "format",
					"PLY format declaration must occur once and declare version 1.0.");
				return std::nullopt;
			}
			if (values[1] == "ascii") {
				header.encoding = Encoding::Ascii;
			} else if (values[1] == "binary_little_endian") {
				header.encoding = Encoding::BinaryLittleEndian;
			} else if (values[1] == "binary_big_endian") {
				header.encoding = Encoding::BinaryBigEndian;
			} else {
				issues.emplace_back(
					IssueCode::UnsupportedEncoding, "format",
					"PLY encoding is outside the PLY 1.0 decoder contract.");
				return std::nullopt;
			}
			saw_format = true;
			continue;
		}
		if (values[0] == "element") {
			if (values.size() != 3u) {
				issues.emplace_back(
					IssueCode::MalformedHeader, "element",
					"PLY element declaration is malformed.");
				return std::nullopt;
			}
			std::uint64_t count = 0u;
			try {
				std::size_t consumed = 0u;
				count = std::stoull(values[2], &consumed);
				if (consumed != values[2].size()) throw std::invalid_argument("count");
			} catch (...) {
				issues.emplace_back(
					IssueCode::MalformedHeader, values[1],
					"PLY element count is not an unsigned integer.");
				return std::nullopt;
			}
			current_element = values[1];
			if (current_element == "vertex") {
				if (saw_vertex || positive_element_before_vertex) {
					issues.emplace_back(
						IssueCode::MalformedHeader, "vertex",
						"PLY vertex data must be the first positive-count element.");
					return std::nullopt;
				}
				saw_vertex = true;
				header.vertex_count = count;
			} else if (!saw_vertex && count > 0u) {
				positive_element_before_vertex = true;
			}
			continue;
		}
		if (values[0] == "property") {
			if (current_element.empty()) {
				issues.emplace_back(
					IssueCode::MalformedHeader, "property",
					"PLY property must belong to a declared element.");
				return std::nullopt;
			}
			if (current_element != "vertex") continue;
			if (values.size() >= 2u && values[1] == "list") {
				issues.emplace_back(
					IssueCode::UnsupportedPropertyType, "vertex",
					"PLY vertex list properties are not admitted by D3A.");
				return std::nullopt;
			}
			if (values.size() != 3u) {
				issues.emplace_back(
					IssueCode::MalformedHeader, "vertex-property",
					"PLY scalar vertex property declaration is malformed.");
				return std::nullopt;
			}
			const auto byte_size = scalar_size(values[1]);
			if (!byte_size.has_value()) {
				issues.emplace_back(
					IssueCode::UnsupportedPropertyType, values[2],
					"PLY vertex property uses an unsupported scalar type.");
				return std::nullopt;
			}
			if (std::any_of(
				    header.vertex_properties.begin(),
				    header.vertex_properties.end(),
				    [&](const PlyScalarProperty &property) {
					    return property.name == values[2];
				    })) {
				issues.emplace_back(
					IssueCode::MalformedHeader, values[2],
					"PLY vertex property names must be unique.");
				return std::nullopt;
			}
			header.vertex_properties.push_back(
				PlyScalarProperty{values[1], values[2], *byte_size});
			header.point_record_length_bytes += *byte_size;
			continue;
		}
		if (values[0] == "end_header") {
			if (values.size() != 1u) {
				issues.emplace_back(
					IssueCode::MalformedHeader, "end_header",
					"PLY end_header declaration must not contain extra tokens.");
				return std::nullopt;
			}
			header.point_data_offset = line_start;
			ended = true;
			break;
		}
		issues.emplace_back(
			IssueCode::MalformedHeader, "line-" + std::to_string(line_number),
			"PLY header contains an unknown declaration.");
		return std::nullopt;
	}
	if (!saw_format || !ended) {
		issues.emplace_back(
			IssueCode::MalformedHeader, "header",
			"PLY header is missing format or end_header.");
		return std::nullopt;
	}
	if (!saw_vertex) {
		issues.emplace_back(
			IssueCode::MissingVertexElement, "header",
			"PLY header does not declare a vertex element.");
		return std::nullopt;
	}
	const auto has_property = [&](const std::string &name) {
		return std::any_of(
			header.vertex_properties.begin(), header.vertex_properties.end(),
			[&](const PlyScalarProperty &property) {
				return property.name == name;
			});
	};
	if (!has_property("x") || !has_property("y") || !has_property("z")) {
		issues.emplace_back(
			IssueCode::MissingCoordinateProperty, "vertex",
			"PLY vertex element requires x, y, and z scalar properties.");
		return std::nullopt;
	}
	return header;
}

VegetationPointCloudBounds3d observed_bounds(
	const std::vector<VegetationPointCloudPointRecord> &points)
{
	double minimum_x = points.front().positionMetres().x();
	double minimum_y = points.front().positionMetres().y();
	double minimum_z = points.front().positionMetres().z();
	double maximum_x = minimum_x;
	double maximum_y = minimum_y;
	double maximum_z = minimum_z;
	for (const auto &point : points) {
		const auto &position = point.positionMetres();
		minimum_x = std::min(minimum_x, position.x());
		minimum_y = std::min(minimum_y, position.y());
		minimum_z = std::min(minimum_z, position.z());
		maximum_x = std::max(maximum_x, position.x());
		maximum_y = std::max(maximum_y, position.y());
		maximum_z = std::max(maximum_z, position.z());
	}
	return VegetationPointCloudBounds3d(
		VegetationMeasuredPoint3d(minimum_x, minimum_y, minimum_z),
		VegetationMeasuredPoint3d(maximum_x, maximum_y, maximum_z));
}

}

VegetationPointCloudIngestionReport VegetationPlyPointCloudDecoder::decode(
	const VegetationMeasuredSourceArtifact &artifact,
	const VegetationPointCloudIngestionContext &context) const
{
	std::vector<Issue> issues =
		VegetationPointCloudSourceArtifactValidationService().validate(
			artifact, context, source_media_type, {source_schema_version});
	std::vector<Observation> observations;
	if (!issues.empty()) {
		return report(
			std::move(issues), std::move(observations), std::nullopt,
			std::nullopt);
	}
	const auto header = parse_header(artifact.sourcePayload(), issues);
	if (!header.has_value()) {
		return report(
			std::move(issues), std::move(observations), std::nullopt,
			std::nullopt);
	}
	if (header->vertex_count > context.policy().maximumPointCount()) {
		issues.emplace_back(
			IssueCode::PointCountLimitExceeded, "vertex",
			"PLY vertex count exceeds the ingestion policy boundary.");
		return report(
			std::move(issues), std::move(observations), std::nullopt,
			std::nullopt);
	}
	std::vector<std::string> property_names;
	for (const auto &property : header->vertex_properties) {
		property_names.push_back(property.name);
	}
	auto metadata = VegetationPointCloudSourceMetadata(
		metadata_schema_version, artifact.sourceIdentifier(),
		VegetationPointCloudSourceFormat::Ply, "1.0", header->encoding,
		header->vertex_count, header->point_record_length_bytes,
		std::move(property_names), artifact.coordinateSystem(),
		context.sourceCoordinateUnit(), {{1.0, 1.0, 1.0}}, {{0.0, 0.0, 0.0}},
		std::nullopt, artifact.payloadSha256());
	if (header->encoding != Encoding::Ascii) {
		if (header->vertex_count >
		    (std::numeric_limits<std::size_t>::max() -
		     header->point_data_offset) /
			    header->point_record_length_bytes) {
			issues.emplace_back(
				IssueCode::PointCountLimitExceeded, "vertex",
				"PLY binary point byte range overflows the host address space.");
			return report(
				std::move(issues), std::move(observations), std::move(metadata),
				std::nullopt);
		}
		const std::size_t required_size = header->point_data_offset +
			static_cast<std::size_t>(header->vertex_count) *
				header->point_record_length_bytes;
		if (required_size > artifact.sourcePayload().size()) {
			issues.emplace_back(
				IssueCode::TruncatedPointData, "vertex",
				"PLY binary payload ends before the declared vertex records.");
			return report(
				std::move(issues), std::move(observations), std::move(metadata),
				std::nullopt);
		}
	}

	std::set<std::string> deferred_properties;
	for (const auto &property : header->vertex_properties) {
		if (property.name != "x" && property.name != "y" &&
		    property.name != "z" && property.name != "red" &&
		    property.name != "green" && property.name != "blue" &&
		    property.name != "intensity" && property.name != "classification" &&
		    property.name != "return_number" &&
		    property.name != "vegetation_class") {
			deferred_properties.insert(property.name);
		}
	}
	for (const auto &property : deferred_properties) {
		observations.emplace_back(
			property, ObservationKind::DeferredProperty,
			"PLY vertex property is preserved in source provenance but is not mapped by D3A.");
	}

	std::istringstream point_stream(
		artifact.sourcePayload().substr(header->point_data_offset));
	std::size_t binary_offset = header->point_data_offset;
	std::vector<VegetationPointCloudPointRecord> points;
	points.reserve(static_cast<std::size_t>(header->vertex_count));
	std::set<std::tuple<double, double, double>> unique_positions;
	for (std::uint64_t point_index = 0u; point_index < header->vertex_count;
	     ++point_index) {
		std::optional<double> x;
		std::optional<double> y;
		std::optional<double> z;
		std::optional<std::uint16_t> red;
		std::optional<std::uint16_t> green;
		std::optional<std::uint16_t> blue;
		std::optional<std::uint16_t> intensity;
		std::optional<std::uint8_t> classification;
		std::optional<std::uint8_t> return_number;
		OrganClass organ_class = OrganClass::Unknown;
		bool record_valid = true;
		const std::string record_identifier =
			"vertex-" + std::to_string(point_index);
		for (const auto &property : header->vertex_properties) {
			std::string value;
			if (header->encoding == Encoding::Ascii) {
				if (!(point_stream >> value)) {
					issues.emplace_back(
						IssueCode::TruncatedPointData, record_identifier,
						"PLY point data ended before the declared vertex count.");
					return report(
						std::move(issues), std::move(observations),
						std::move(metadata), std::nullopt);
				}
			} else {
				const auto binary_value = read_binary_scalar(
					artifact.sourcePayload(), binary_offset, property.type,
					header->encoding);
				if (!binary_value.has_value()) {
					issues.emplace_back(
						IssueCode::TruncatedPointData, record_identifier,
						"PLY binary vertex contains truncated or non-finite scalar data.");
					return report(
						std::move(issues), std::move(observations),
						std::move(metadata), std::nullopt);
				}
				value = scalar_text(*binary_value);
			}
			if (property.name == "x" || property.name == "y" ||
			    property.name == "z") {
				const auto number = parse_number(value);
				if (!number.has_value()) {
					issues.emplace_back(
						IssueCode::NonFiniteCoordinate, record_identifier,
						"PLY coordinate is not a finite numeric value.");
					record_valid = false;
				} else if (property.name == "x") {
					x = *number;
				} else if (property.name == "y") {
					y = *number;
				} else {
					z = *number;
				}
			} else if (property.name == "red") {
				red = parse_uint16(value);
				record_valid = record_valid && red.has_value();
			} else if (property.name == "green") {
				green = parse_uint16(value);
				record_valid = record_valid && green.has_value();
			} else if (property.name == "blue") {
				blue = parse_uint16(value);
				record_valid = record_valid && blue.has_value();
			} else if (property.name == "intensity") {
				intensity = parse_uint16(value);
				record_valid = record_valid && intensity.has_value();
			} else if (property.name == "classification") {
				classification = parse_uint8(value);
				record_valid = record_valid && classification.has_value();
			} else if (property.name == "return_number") {
				return_number = parse_uint8(value);
				record_valid = record_valid && return_number.has_value();
			} else if (property.name == "vegetation_class") {
				const auto value_class = parse_uint8(value);
				if (!value_class.has_value() || *value_class > 2u) {
					record_valid = false;
				} else if (*value_class == 1u) {
					organ_class = OrganClass::Woody;
				} else if (*value_class == 2u) {
					organ_class = OrganClass::Foliage;
				}
			}
		}
		if (!record_valid || !x.has_value() || !y.has_value() || !z.has_value()) {
			issues.emplace_back(
				IssueCode::UnsupportedPropertyType, record_identifier,
				"PLY vertex contains a value outside its admitted semantic range.");
			observations.emplace_back(
				record_identifier, ObservationKind::RejectedPoint,
				"PLY vertex was rejected before canonical coordinate mapping.");
			continue;
		}
		const auto mapped = VegetationMeasuredPointCanonicalizationService()
			.mapToCanonicalPlantSpace(
				VegetationMeasuredPoint3d(*x, *y, *z), artifact.coordinateSystem(),
				context.sourceCoordinateUnit(),
				context.coordinateReferenceTransform());
		if (!mapped.succeeded()) {
			issues.emplace_back(
				IssueCode::InvalidCoordinateReferenceTransform, record_identifier,
				mapped.rejectionReason());
			observations.emplace_back(
				record_identifier, ObservationKind::RejectedPoint,
				"PLY vertex could not be mapped into canonical plant space.");
			continue;
		}
		const auto &position = *mapped.transformedPoint();
		const auto position_key =
			std::make_tuple(position.x(), position.y(), position.z());
		if (!unique_positions.insert(position_key).second &&
		    context.policy().rejectDuplicatePoints()) {
			issues.emplace_back(
				IssueCode::DuplicatePoint, record_identifier,
				"PLY vertex duplicates an earlier canonical point.");
			observations.emplace_back(
				record_identifier, ObservationKind::RejectedPoint,
				"Duplicate PLY vertex was rejected by ingestion policy.");
			continue;
		}
		points.emplace_back(
			static_cast<std::size_t>(point_index), position, red, green, blue,
			intensity, classification, return_number, organ_class);
	}
	if (!issues.empty() || points.empty()) {
		if (points.empty() && issues.empty()) {
			issues.emplace_back(
				IssueCode::TruncatedPointData, "vertex",
				"PLY payload contains no admitted point records.");
		}
		return report(
			std::move(issues), std::move(observations), std::move(metadata),
			std::nullopt);
	}
	observations.emplace_back(
		"vertex", ObservationKind::AcceptedPoint,
		std::to_string(points.size()) +
			" PLY vertices were mapped into canonical plant space.");
	const VegetationPointCloudBounds3d bounds = observed_bounds(points);
	auto dataset = VegetationPointCloudDataset(
		context.datasetIdentifier(), metadata, std::move(points), bounds);
	return report(
		std::move(issues), std::move(observations), std::move(metadata),
		std::move(dataset));
}
