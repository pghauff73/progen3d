#pragma once

enum class VegetationPointCloudSourceFormat
{
	Ply,
	Las,
	Laz,
	E57
};

inline const char *vegetationPointCloudSourceFormatName(
	VegetationPointCloudSourceFormat format)
{
	switch (format) {
	case VegetationPointCloudSourceFormat::Ply: return "PLY";
	case VegetationPointCloudSourceFormat::Las: return "LAS";
	case VegetationPointCloudSourceFormat::Laz: return "LAZ";
	case VegetationPointCloudSourceFormat::E57: return "E57";
	}
	return "Unknown";
}

enum class VegetationPointCloudEncoding
{
	Ascii,
	BinaryLittleEndian,
	BinaryBigEndian,
	Compressed
};

inline const char *vegetationPointCloudEncodingName(
	VegetationPointCloudEncoding encoding)
{
	switch (encoding) {
	case VegetationPointCloudEncoding::Ascii: return "Ascii";
	case VegetationPointCloudEncoding::BinaryLittleEndian:
		return "BinaryLittleEndian";
	case VegetationPointCloudEncoding::BinaryBigEndian:
		return "BinaryBigEndian";
	case VegetationPointCloudEncoding::Compressed: return "Compressed";
	}
	return "Unknown";
}

enum class VegetationPointCloudOrganClass
{
	Unknown,
	Woody,
	Foliage
};

inline const char *vegetationPointCloudOrganClassName(
	VegetationPointCloudOrganClass organ_class)
{
	switch (organ_class) {
	case VegetationPointCloudOrganClass::Unknown: return "Unknown";
	case VegetationPointCloudOrganClass::Woody: return "Woody";
	case VegetationPointCloudOrganClass::Foliage: return "Foliage";
	}
	return "Unknown";
}
