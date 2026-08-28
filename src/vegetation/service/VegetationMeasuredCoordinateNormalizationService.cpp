#include "vegetation/service/VegetationMeasuredCoordinateNormalizationService.h"

#include <cmath>
#include <optional>
#include <string>

namespace {

constexpr const char *canonical_coordinate_system = "LocalPlantXYZ-ZUp";

bool finite_point(const VegetationMeasuredPoint3d &point)
{
	return std::isfinite(point.x()) && std::isfinite(point.y()) &&
	       std::isfinite(point.z());
}

}

bool VegetationMeasuredCoordinateNormalizationService::supportsCoordinateSystem(
	const std::string &source_coordinate_system) const
{
	return source_coordinate_system == "LocalPlantXYZ-ZUp" ||
	       source_coordinate_system == "LocalPlantXZY-YUp";
}

VegetationMeasuredCoordinateNormalizationResult
VegetationMeasuredCoordinateNormalizationService::normalize(
	const VegetationMeasuredPoint3d &source_point,
	const std::string &source_coordinate_system) const
{
	if (!finite_point(source_point)) {
		return VegetationMeasuredCoordinateNormalizationResult(
			std::nullopt,
			canonical_coordinate_system,
			"Measured source point contains a non-finite coordinate.");
	}
	if (source_coordinate_system == "LocalPlantXYZ-ZUp") {
		return VegetationMeasuredCoordinateNormalizationResult(
			source_point, canonical_coordinate_system, std::string());
	}
	if (source_coordinate_system == "LocalPlantXZY-YUp") {
		return VegetationMeasuredCoordinateNormalizationResult(
			VegetationMeasuredPoint3d(
				source_point.x(), source_point.z(), source_point.y()),
			canonical_coordinate_system,
			std::string());
	}
	return VegetationMeasuredCoordinateNormalizationResult(
		std::nullopt,
		canonical_coordinate_system,
		"Measured source coordinate system is unsupported.");
}
