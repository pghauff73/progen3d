#include "vegetation/service/VegetationPointCloudFormatCapabilityCatalog.h"

#include <algorithm>
#include <string>
#include <vector>

std::vector<VegetationPointCloudFormatCapability>
VegetationPointCloudFormatCapabilityCatalog::capabilities() const
{
	using Encoding = VegetationPointCloudEncoding;
	using Format = VegetationPointCloudSourceFormat;
	using Level = VegetationPointCloudCapabilityLevel;

	std::vector<VegetationPointCloudFormatCapability> values;
	values.emplace_back(
		Format::Ply, "application/ply", "PLY-1.0", Encoding::Ascii,
		Level::PointRecords, std::string(),
		"Scalar vertex records are decoded; vertex list properties are rejected.");
	values.emplace_back(
		Format::Ply, "application/ply", "PLY-1.0",
		Encoding::BinaryLittleEndian, Level::PointRecords, std::string(),
		"Scalar little-endian vertex records are decoded; vertex list properties are rejected.");
	values.emplace_back(
		Format::Ply, "application/ply", "PLY-1.0",
		Encoding::BinaryBigEndian, Level::PointRecords, std::string(),
		"Scalar big-endian vertex records are decoded; vertex list properties are rejected.");
	for (const char *schema : {
		     "LAS-1.0", "LAS-1.1", "LAS-1.2", "LAS-1.3", "LAS-1.4"}) {
		values.emplace_back(
			Format::Las, "application/vnd.las", schema,
			Encoding::BinaryLittleEndian, Level::PointRecords, std::string(),
			"Uncompressed LAS point data records are decoded within policy bounds.");
	}
	values.emplace_back(
		Format::Las, "application/vnd.las", "LAS-1.5",
		Encoding::BinaryLittleEndian, Level::Unsupported, std::string(),
		"LAS 1.5 is newer than the frozen D3A decoder contract and is rejected.");
	values.emplace_back(
		Format::Laz, "application/vnd.laz", "LAZ-1.4", Encoding::Compressed,
		Level::Unsupported, "LASzip or PDAL",
		"Compressed LAZ records require an audited external decoder.");
	values.emplace_back(
		Format::E57, "model/e57", "E57-ASTM-E2807", Encoding::Compressed,
		Level::Unsupported, "libE57Format",
		"E57 XML and compressed binary packets require the reference library.");
	return values;
}

std::optional<VegetationPointCloudFormatCapability>
VegetationPointCloudFormatCapabilityCatalog::findCapability(
	const std::string &media_type,
	const std::string &source_schema_version,
	VegetationPointCloudEncoding encoding) const
{
	const auto values = capabilities();
	const auto match = std::find_if(
		values.begin(), values.end(), [&](const auto &capability) {
			return capability.mediaType() == media_type &&
			       capability.sourceSchemaVersion() == source_schema_version &&
			       capability.encoding() == encoding;
		});
	if (match == values.end()) return std::nullopt;
	return *match;
}
