#include "vegetation/service/VegetationLasPointCloudDecoder.h"

#include "vegetation/service/VegetationMeasuredPointCanonicalizationService.h"
#include "vegetation/service/VegetationPointCloudSourceArtifactValidationService.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using Issue = VegetationPointCloudIngestionIssue;
using IssueCode = VegetationPointCloudIngestionIssueCode;
using Observation = VegetationPointCloudIngestionObservation;
using ObservationKind = VegetationPointCloudIngestionObservationKind;

constexpr const char *decoder_identifier = "VegetationLasPointCloudDecoder";
constexpr const char *source_media_type = "application/vnd.las";
constexpr const char *metadata_schema_version =
	"ProGen3D-VegetationPointCloudSourceMetadata-v1";

const std::vector<std::string> supported_schemas = {
	"LAS-1.0", "LAS-1.1", "LAS-1.2", "LAS-1.3", "LAS-1.4"};

struct LasHeader
{
	std::uint8_t version_minor = 0u;
	std::uint16_t header_size = 0u;
	std::uint32_t point_data_offset = 0u;
	std::uint8_t point_format = 0u;
	std::uint16_t point_record_length = 0u;
	std::uint64_t point_count = 0u;
	std::array<double, 3> scales{{1.0, 1.0, 1.0}};
	std::array<double, 3> offsets{{0.0, 0.0, 0.0}};
	VegetationPointCloudBounds3d source_bounds{
		VegetationMeasuredPoint3d(0.0, 0.0, 0.0),
		VegetationMeasuredPoint3d(0.0, 0.0, 0.0)};
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

std::uint16_t read_uint16(const std::string &payload, std::size_t offset)
{
	return static_cast<std::uint16_t>(
		static_cast<std::uint8_t>(payload[offset])) |
	       static_cast<std::uint16_t>(
		       static_cast<std::uint8_t>(payload[offset + 1u]))
		       << 8u;
}

std::uint32_t read_uint32(const std::string &payload, std::size_t offset)
{
	std::uint32_t value = 0u;
	for (std::size_t byte_index = 0u; byte_index < 4u; ++byte_index) {
		value |= static_cast<std::uint32_t>(
			         static_cast<std::uint8_t>(payload[offset + byte_index]))
		         << (8u * byte_index);
	}
	return value;
}

std::uint64_t read_uint64(const std::string &payload, std::size_t offset)
{
	std::uint64_t value = 0u;
	for (std::size_t byte_index = 0u; byte_index < 8u; ++byte_index) {
		value |= static_cast<std::uint64_t>(
			         static_cast<std::uint8_t>(payload[offset + byte_index]))
		         << (8u * byte_index);
	}
	return value;
}

std::int32_t read_int32(const std::string &payload, std::size_t offset)
{
	const std::uint32_t value = read_uint32(payload, offset);
	std::int32_t signed_value = 0;
	std::memcpy(&signed_value, &value, sizeof(signed_value));
	return signed_value;
}

double read_double(const std::string &payload, std::size_t offset)
{
	const std::uint64_t bits = read_uint64(payload, offset);
	double value = 0.0;
	std::memcpy(&value, &bits, sizeof(value));
	return value;
}

std::size_t minimum_header_size(std::uint8_t minor_version)
{
	if (minor_version <= 2u) return 227u;
	if (minor_version == 3u) return 235u;
	return 375u;
}

std::uint8_t maximum_point_format(std::uint8_t minor_version)
{
	if (minor_version == 0u) return 0u;
	if (minor_version == 1u) return 1u;
	if (minor_version == 2u) return 3u;
	if (minor_version == 3u) return 5u;
	return 10u;
}

std::optional<std::size_t> minimum_point_record_length(
	std::uint8_t point_format)
{
	static constexpr std::array<std::size_t, 11> lengths = {
		20u, 28u, 26u, 34u, 57u, 63u, 30u, 36u, 38u, 59u, 67u};
	if (point_format >= lengths.size()) return std::nullopt;
	return lengths[point_format];
}

std::optional<std::size_t> rgb_offset(std::uint8_t point_format)
{
	switch (point_format) {
	case 2u: return 20u;
	case 3u:
	case 5u: return 28u;
	case 7u:
	case 8u:
	case 10u: return 30u;
	default: return std::nullopt;
	}
}

std::optional<LasHeader> parse_header(
	const std::string &payload,
	const VegetationPointCloudIngestionContext &context,
	std::vector<Issue> &issues)
{
	if (payload.size() < 227u) {
		issues.emplace_back(
			IssueCode::TruncatedPointData, "header",
			"LAS payload is shorter than the minimum public header block.");
		return std::nullopt;
	}
	if (payload.compare(0u, 4u, "LASF") != 0) {
		issues.emplace_back(
			IssueCode::MalformedHeader, "signature",
			"LAS payload must begin with the exact LASF file signature.");
		return std::nullopt;
	}
	const std::uint8_t version_major =
		static_cast<std::uint8_t>(payload[24u]);
	const std::uint8_t version_minor =
		static_cast<std::uint8_t>(payload[25u]);
	if (version_major != 1u || version_minor > 4u) {
		issues.emplace_back(
			IssueCode::UnsupportedSourceSchemaVersion, "version",
			"LAS decoder admits versions 1.0 through 1.4 only; LAS 1.5 is rejected.");
		return std::nullopt;
	}
	const std::string detected_schema =
		"LAS-1." + std::to_string(version_minor);
	if (context.sourceSchemaVersion() != detected_schema) {
		issues.emplace_back(
			IssueCode::UnsupportedSourceSchemaVersion, "version",
			"LAS header version does not match the declared source schema.");
		return std::nullopt;
	}
	LasHeader header;
	header.version_minor = version_minor;
	header.header_size = read_uint16(payload, 94u);
	header.point_data_offset = read_uint32(payload, 96u);
	const std::uint8_t raw_point_format =
		static_cast<std::uint8_t>(payload[104u]);
	if ((raw_point_format & 0xc0u) != 0u) {
		issues.emplace_back(
			IssueCode::UnsupportedEncoding, "point-format",
			"Compressed or reserved LAS point format flags are outside D3A.");
		return std::nullopt;
	}
	header.point_format = raw_point_format & 0x3fu;
	header.point_record_length = read_uint16(payload, 105u);
	if (header.header_size < minimum_header_size(version_minor) ||
	    header.header_size > payload.size()) {
		issues.emplace_back(
			IssueCode::MalformedHeader, "header-size",
			"LAS public header size is invalid for the declared version.");
		return std::nullopt;
	}
	if (header.point_data_offset < header.header_size ||
	    header.point_data_offset > payload.size()) {
		issues.emplace_back(
			IssueCode::MalformedHeader, "point-data-offset",
			"LAS point data offset is outside the payload or precedes the header.");
		return std::nullopt;
	}
	if (header.point_format > maximum_point_format(version_minor)) {
		issues.emplace_back(
			IssueCode::UnsupportedSourceSchemaVersion, "point-format",
			"LAS point data record format is newer than the declared LAS version.");
		return std::nullopt;
	}
	const auto minimum_record_length =
		minimum_point_record_length(header.point_format);
	if (!minimum_record_length.has_value() ||
	    header.point_record_length < *minimum_record_length) {
		issues.emplace_back(
			IssueCode::MalformedHeader, "point-record-length",
			"LAS point record length is too short for its point format.");
		return std::nullopt;
	}
	const std::uint32_t legacy_count = read_uint32(payload, 107u);
	if (version_minor == 4u) {
		const std::uint64_t extended_count = read_uint64(payload, 247u);
		header.point_count = extended_count == 0u ? legacy_count : extended_count;
	} else {
		header.point_count = legacy_count;
	}
	if (header.point_count > context.policy().maximumPointCount()) {
		issues.emplace_back(
			IssueCode::PointCountLimitExceeded, "point-count",
			"LAS point count exceeds the ingestion policy boundary.");
		return std::nullopt;
	}
	if (header.point_count == 0u) {
		issues.emplace_back(
			IssueCode::TruncatedPointData, "point-count",
			"LAS payload declares zero point records.");
		return std::nullopt;
	}
	if (header.point_count >
	    (std::numeric_limits<std::size_t>::max() - header.point_data_offset) /
		    header.point_record_length) {
		issues.emplace_back(
			IssueCode::PointCountLimitExceeded, "point-count",
			"LAS point byte range overflows the host address space.");
		return std::nullopt;
	}
	const std::size_t required_size =
		static_cast<std::size_t>(header.point_data_offset) +
		static_cast<std::size_t>(header.point_count) * header.point_record_length;
	if (required_size > payload.size()) {
		issues.emplace_back(
			IssueCode::TruncatedPointData, "point-data",
			"LAS payload ends before the declared point records are complete.");
		return std::nullopt;
	}
	header.scales = {
		read_double(payload, 131u), read_double(payload, 139u),
		read_double(payload, 147u)};
	header.offsets = {
		read_double(payload, 155u), read_double(payload, 163u),
		read_double(payload, 171u)};
	const double maximum_x = read_double(payload, 179u);
	const double minimum_x = read_double(payload, 187u);
	const double maximum_y = read_double(payload, 195u);
	const double minimum_y = read_double(payload, 203u);
	const double maximum_z = read_double(payload, 211u);
	const double minimum_z = read_double(payload, 219u);
	header.source_bounds = VegetationPointCloudBounds3d(
		VegetationMeasuredPoint3d(minimum_x, minimum_y, minimum_z),
		VegetationMeasuredPoint3d(maximum_x, maximum_y, maximum_z));
	if (!header.source_bounds.valid() ||
	    std::any_of(header.scales.begin(), header.scales.end(), [](double value) {
		    return !std::isfinite(value) || value <= 0.0;
	    }) ||
	    std::any_of(header.offsets.begin(), header.offsets.end(), [](double value) {
		    return !std::isfinite(value);
	    })) {
		issues.emplace_back(
			IssueCode::InvalidDeclaredBounds, "coordinate-metadata",
			"LAS scales, offsets, or declared bounds are invalid.");
		return std::nullopt;
	}
	return header;
}

VegetationPointCloudBounds3d calculate_bounds(
	const std::vector<VegetationMeasuredPoint3d> &positions)
{
	double minimum_x = positions.front().x();
	double minimum_y = positions.front().y();
	double minimum_z = positions.front().z();
	double maximum_x = minimum_x;
	double maximum_y = minimum_y;
	double maximum_z = minimum_z;
	for (const auto &position : positions) {
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

std::optional<VegetationPointCloudBounds3d> map_declared_bounds(
	const LasHeader &header,
	const VegetationMeasuredSourceArtifact &artifact,
	const VegetationPointCloudIngestionContext &context,
	std::vector<Issue> &issues)
{
	std::vector<VegetationMeasuredPoint3d> corners;
	corners.reserve(8u);
	for (const double x : {
		     header.source_bounds.minimum().x(), header.source_bounds.maximum().x()}) {
		for (const double y : {header.source_bounds.minimum().y(),
		                       header.source_bounds.maximum().y()}) {
			for (const double z : {header.source_bounds.minimum().z(),
			                       header.source_bounds.maximum().z()}) {
				const auto mapped = VegetationMeasuredPointCanonicalizationService()
					.mapToCanonicalPlantSpace(
						VegetationMeasuredPoint3d(x, y, z),
						artifact.coordinateSystem(), context.sourceCoordinateUnit(),
						context.coordinateReferenceTransform());
				if (!mapped.succeeded()) {
					issues.emplace_back(
						IssueCode::InvalidCoordinateReferenceTransform,
						"declared-bounds", mapped.rejectionReason());
					return std::nullopt;
				}
				corners.push_back(*mapped.transformedPoint());
			}
		}
	}
	return calculate_bounds(corners);
}

bool bounds_match(
	const VegetationPointCloudBounds3d &declared_bounds,
	const VegetationPointCloudBounds3d &observed_bounds)
{
	const std::array<double, 6> declared = {
		declared_bounds.minimum().x(), declared_bounds.minimum().y(),
		declared_bounds.minimum().z(), declared_bounds.maximum().x(),
		declared_bounds.maximum().y(), declared_bounds.maximum().z()};
	const std::array<double, 6> observed = {
		observed_bounds.minimum().x(), observed_bounds.minimum().y(),
		observed_bounds.minimum().z(), observed_bounds.maximum().x(),
		observed_bounds.maximum().y(), observed_bounds.maximum().z()};
	for (std::size_t index = 0u; index < declared.size(); ++index) {
		const double tolerance =
			1.0e-8 * std::max({1.0, std::abs(declared[index]),
			                   std::abs(observed[index])});
		if (std::abs(declared[index] - observed[index]) > tolerance) return false;
	}
	return true;
}

}

VegetationPointCloudIngestionReport VegetationLasPointCloudDecoder::decode(
	const VegetationMeasuredSourceArtifact &artifact,
	const VegetationPointCloudIngestionContext &context) const
{
	std::vector<Issue> issues =
		VegetationPointCloudSourceArtifactValidationService().validate(
			artifact, context, source_media_type, supported_schemas);
	std::vector<Observation> observations;
	if (!issues.empty()) {
		return report(
			std::move(issues), std::move(observations), std::nullopt,
			std::nullopt);
	}
	const auto header = parse_header(artifact.sourcePayload(), context, issues);
	if (!header.has_value()) {
		return report(
			std::move(issues), std::move(observations), std::nullopt,
			std::nullopt);
	}
	const auto declared_bounds =
		map_declared_bounds(*header, artifact, context, issues);
	if (!declared_bounds.has_value()) {
		return report(
			std::move(issues), std::move(observations), std::nullopt,
			std::nullopt);
	}
	std::vector<std::string> property_names = {
		"x", "y", "z", "intensity", "return_number", "classification"};
	if (rgb_offset(header->point_format).has_value()) {
		property_names.insert(property_names.end(), {"red", "green", "blue"});
	}
	auto metadata = VegetationPointCloudSourceMetadata(
		metadata_schema_version, artifact.sourceIdentifier(),
		VegetationPointCloudSourceFormat::Las,
		"1." + std::to_string(header->version_minor),
		VegetationPointCloudEncoding::BinaryLittleEndian, header->point_count,
		header->point_record_length, std::move(property_names),
		artifact.coordinateSystem(), context.sourceCoordinateUnit(), header->scales,
		header->offsets, *declared_bounds, artifact.payloadSha256());

	std::vector<VegetationPointCloudPointRecord> points;
	points.reserve(static_cast<std::size_t>(header->point_count));
	std::vector<VegetationMeasuredPoint3d> positions;
	positions.reserve(static_cast<std::size_t>(header->point_count));
	std::set<std::tuple<double, double, double>> unique_positions;
	const auto colour_offset = rgb_offset(header->point_format);
	for (std::uint64_t point_index = 0u; point_index < header->point_count;
	     ++point_index) {
		const std::size_t record_offset =
			static_cast<std::size_t>(header->point_data_offset) +
			static_cast<std::size_t>(point_index) * header->point_record_length;
		const VegetationMeasuredPoint3d source_point(
			static_cast<double>(read_int32(artifact.sourcePayload(), record_offset)) *
					header->scales[0] +
				header->offsets[0],
			static_cast<double>(
				read_int32(artifact.sourcePayload(), record_offset + 4u)) *
					header->scales[1] +
				header->offsets[1],
			static_cast<double>(
				read_int32(artifact.sourcePayload(), record_offset + 8u)) *
					header->scales[2] +
				header->offsets[2]);
		const auto mapped = VegetationMeasuredPointCanonicalizationService()
			.mapToCanonicalPlantSpace(
				source_point, artifact.coordinateSystem(),
				context.sourceCoordinateUnit(),
				context.coordinateReferenceTransform());
		const std::string record_identifier =
			"point-" + std::to_string(point_index);
		if (!mapped.succeeded()) {
			issues.emplace_back(
				IssueCode::InvalidCoordinateReferenceTransform, record_identifier,
				mapped.rejectionReason());
			observations.emplace_back(
				record_identifier, ObservationKind::RejectedPoint,
				"LAS point could not be mapped into canonical plant space.");
			continue;
		}
		const auto &position = *mapped.transformedPoint();
		if (!std::isfinite(position.x()) || !std::isfinite(position.y()) ||
		    !std::isfinite(position.z())) {
			issues.emplace_back(
				IssueCode::NonFiniteCoordinate, record_identifier,
				"LAS point maps to a non-finite canonical coordinate.");
			continue;
		}
		const auto position_key =
			std::make_tuple(position.x(), position.y(), position.z());
		if (!unique_positions.insert(position_key).second &&
		    context.policy().rejectDuplicatePoints()) {
			issues.emplace_back(
				IssueCode::DuplicatePoint, record_identifier,
				"LAS point duplicates an earlier canonical point.");
			observations.emplace_back(
				record_identifier, ObservationKind::RejectedPoint,
				"Duplicate LAS point was rejected by ingestion policy.");
			continue;
		}
		const std::uint16_t intensity =
			read_uint16(artifact.sourcePayload(), record_offset + 12u);
		const std::uint8_t return_flags = static_cast<std::uint8_t>(
			artifact.sourcePayload()[record_offset + 14u]);
		const bool modern_point_format = header->point_format >= 6u;
		const std::uint8_t return_number =
			return_flags & (modern_point_format ? 0x0fu : 0x07u);
		const std::uint8_t raw_classification = static_cast<std::uint8_t>(
			artifact.sourcePayload()[record_offset +
				(modern_point_format ? 16u : 15u)]);
		const std::uint8_t classification = modern_point_format
			                                    ? raw_classification
			                                    : raw_classification & 0x1fu;
		std::optional<std::uint16_t> red;
		std::optional<std::uint16_t> green;
		std::optional<std::uint16_t> blue;
		if (colour_offset.has_value()) {
			red = read_uint16(
				artifact.sourcePayload(), record_offset + *colour_offset);
			green = read_uint16(
				artifact.sourcePayload(), record_offset + *colour_offset + 2u);
			blue = read_uint16(
				artifact.sourcePayload(), record_offset + *colour_offset + 4u);
		}
		points.emplace_back(
			static_cast<std::size_t>(point_index), position, red, green, blue,
			intensity, classification, return_number,
			VegetationPointCloudOrganClass::Unknown);
		positions.push_back(position);
	}
	if (!issues.empty() || points.empty()) {
		if (points.empty() && issues.empty()) {
			issues.emplace_back(
				IssueCode::TruncatedPointData, "point-data",
				"LAS payload contains no admitted point records.");
		}
		return report(
			std::move(issues), std::move(observations), std::move(metadata),
			std::nullopt);
	}
	const VegetationPointCloudBounds3d observed = calculate_bounds(positions);
	if (!bounds_match(*declared_bounds, observed)) {
		issues.emplace_back(
			IssueCode::DeclaredBoundsMismatch, "declared-bounds",
			"LAS public-header bounds do not match the decoded point extent.");
		return report(
			std::move(issues), std::move(observations), std::move(metadata),
			std::nullopt);
	}
	observations.emplace_back(
		"point-data", ObservationKind::AcceptedPoint,
		std::to_string(points.size()) +
			" LAS points were mapped into canonical plant space.");
	observations.emplace_back(
		"classification", ObservationKind::DeferredProperty,
		"LAS vegetation height classes are preserved but are not interpreted as woody or foliage organ labels.");
	auto dataset = VegetationPointCloudDataset(
		context.datasetIdentifier(), metadata, std::move(points), observed);
	return report(
		std::move(issues), std::move(observations), std::move(metadata),
		std::move(dataset));
}
