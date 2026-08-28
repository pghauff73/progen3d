#include "vegetation/service/VegetationMeasuredPointCanonicalizationService.h"

#include "vegetation/service/VegetationMeasuredCoordinateNormalizationService.h"
#include "vegetation/service/VegetationMeasuredCoordinateReferenceTransformService.h"
#include "vegetation/service/VegetationMeasurementUnitNormalizationService.h"

#include <optional>
#include <string>

namespace {

constexpr const char *canonical_coordinate_system = "LocalPlantXYZ-ZUp";

VegetationMeasuredCoordinateReferenceTransformResult rejected(
	const std::string &reason)
{
	return VegetationMeasuredCoordinateReferenceTransformResult(
		std::nullopt, canonical_coordinate_system, "m", reason);
}

}

VegetationMeasuredCoordinateReferenceTransformResult
VegetationMeasuredPointCanonicalizationService::mapToCanonicalPlantSpace(
	const VegetationMeasuredPoint3d &source_point,
	const std::string &source_coordinate_system,
	const std::string &source_coordinate_unit,
	const std::optional<VegetationMeasuredCoordinateReferenceTransform>
		&coordinate_reference_transform) const
{
	if (coordinate_reference_transform.has_value()) {
		const auto &transform = *coordinate_reference_transform;
		if (transform.sourceCoordinateSystem() != source_coordinate_system) {
			return rejected(
				"Coordinate reference transform source frame does not match the artifact.");
		}
		if (transform.sourceCoordinateUnit() != source_coordinate_unit) {
			return rejected(
				"Coordinate reference transform source unit does not match the decoded point unit.");
		}
		return VegetationMeasuredCoordinateReferenceTransformService()
			.mapSourceToTarget(source_point, transform);
	}

	const VegetationMeasurementUnitNormalizationService unit_normalization;
	const auto x = unit_normalization.normalize(
		source_point.x(), source_coordinate_unit,
		VegetationMeasurementQuantityKind::LengthMetres);
	const auto y = unit_normalization.normalize(
		source_point.y(), source_coordinate_unit,
		VegetationMeasurementQuantityKind::LengthMetres);
	const auto z = unit_normalization.normalize(
		source_point.z(), source_coordinate_unit,
		VegetationMeasurementQuantityKind::LengthMetres);
	if (!x.succeeded() || !y.succeeded() || !z.succeeded()) {
		return rejected("Decoded point uses an unsupported length unit.");
	}
	const VegetationMeasuredCoordinateNormalizationResult normalized =
		VegetationMeasuredCoordinateNormalizationService().normalize(
			VegetationMeasuredPoint3d(
				*x.normalizedValue(), *y.normalizedValue(), *z.normalizedValue()),
			source_coordinate_system);
	if (!normalized.succeeded()) return rejected(normalized.rejectionReason());
	return VegetationMeasuredCoordinateReferenceTransformResult(
		*normalized.normalizedPoint(), normalized.canonicalCoordinateSystem(), "m",
		std::string());
}
