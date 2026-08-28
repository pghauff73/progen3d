#include "vegetation/service/VegetationMeasuredCoordinateReferenceTransformService.h"

#include "vegetation/service/VegetationMeasurementUnitNormalizationService.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <optional>
#include <string>
#include <vector>

namespace {

using Issue = VegetationMeasuredCoordinateReferenceTransformIssue;
using IssueCode = VegetationMeasuredCoordinateReferenceTransformIssueCode;

constexpr const char *transform_schema =
	"ProGen3D-VegetationMeasuredCoordinateReferenceTransform-v1";
constexpr const char *canonical_coordinate_system = "LocalPlantXYZ-ZUp";

bool is_sha256(const std::string &value)
{
	return value.size() == 64u &&
	       std::all_of(
		       value.begin(), value.end(), [](unsigned char character) {
			       return std::isxdigit(character) != 0;
		       });
}

bool supported_length_unit(const std::string &unit)
{
	return unit == "m" || unit == "cm" || unit == "mm";
}

double determinant(const std::array<double, 16> &matrix)
{
	return matrix[0] * (matrix[5] * matrix[10] - matrix[6] * matrix[9]) -
	       matrix[1] * (matrix[4] * matrix[10] - matrix[6] * matrix[8]) +
	       matrix[2] * (matrix[4] * matrix[9] - matrix[5] * matrix[8]);
}

std::array<double, 9> inverse_linear_matrix(
	const std::array<double, 16> &matrix)
{
	const double inverse_determinant = 1.0 / determinant(matrix);
	return {
		(matrix[5] * matrix[10] - matrix[6] * matrix[9]) * inverse_determinant,
		(matrix[2] * matrix[9] - matrix[1] * matrix[10]) * inverse_determinant,
		(matrix[1] * matrix[6] - matrix[2] * matrix[5]) * inverse_determinant,
		(matrix[6] * matrix[8] - matrix[4] * matrix[10]) * inverse_determinant,
		(matrix[0] * matrix[10] - matrix[2] * matrix[8]) * inverse_determinant,
		(matrix[2] * matrix[4] - matrix[0] * matrix[6]) * inverse_determinant,
		(matrix[4] * matrix[9] - matrix[5] * matrix[8]) * inverse_determinant,
		(matrix[1] * matrix[8] - matrix[0] * matrix[9]) * inverse_determinant,
		(matrix[0] * matrix[5] - matrix[1] * matrix[4]) * inverse_determinant,
	};
}

double metres_to_source_unit(double metres, const std::string &source_unit)
{
	if (source_unit == "cm") return metres * 100.0;
	if (source_unit == "mm") return metres * 1000.0;
	return metres;
}

VegetationMeasuredCoordinateReferenceTransformResult rejected(
	const std::string &target_coordinate_system,
	const std::string &target_coordinate_unit,
	const std::string &reason)
{
	return VegetationMeasuredCoordinateReferenceTransformResult(
		std::nullopt, target_coordinate_system, target_coordinate_unit, reason);
}

}

VegetationMeasuredCoordinateReferenceTransformValidationReport
VegetationMeasuredCoordinateReferenceTransformService::validate(
	const VegetationMeasuredCoordinateReferenceTransform &transform) const
{
	std::vector<Issue> issues;
	if (transform.schemaVersion() != transform_schema) {
		issues.emplace_back(
			IssueCode::UnsupportedSchema,
			"Coordinate reference transform schema is unsupported.");
	}
	if (transform.transformIdentifier().empty()) {
		issues.emplace_back(
			IssueCode::EmptyTransformIdentifier,
			"Coordinate reference transform requires an identifier.");
	}
	if (transform.sourceCoordinateSystem().empty()) {
		issues.emplace_back(
			IssueCode::EmptySourceCoordinateSystem,
			"Coordinate reference transform requires a source coordinate system.");
	}
	if (transform.targetCoordinateSystem() != canonical_coordinate_system) {
		issues.emplace_back(
			IssueCode::UnsupportedTargetCoordinateSystem,
			"Coordinate reference transform target must be LocalPlantXYZ-ZUp.");
	}
	if (!supported_length_unit(transform.sourceCoordinateUnit())) {
		issues.emplace_back(
			IssueCode::UnsupportedSourceCoordinateUnit,
			"Coordinate reference transform source unit must be m, cm, or mm.");
	}
	if (transform.targetCoordinateUnit() != "m") {
		issues.emplace_back(
			IssueCode::UnsupportedTargetCoordinateUnit,
			"Coordinate reference transform target unit must be metres.");
	}
	const auto &matrix = transform.sourceMetresToTargetMetres();
	if (!std::all_of(matrix.begin(), matrix.end(), [](double value) {
		    return std::isfinite(value);
	    })) {
		issues.emplace_back(
			IssueCode::NonFiniteMatrix,
			"Coordinate reference transform matrix must contain finite values.");
	}
	if (std::abs(matrix[12]) > 1.0e-12 ||
	    std::abs(matrix[13]) > 1.0e-12 ||
	    std::abs(matrix[14]) > 1.0e-12 ||
	    std::abs(matrix[15] - 1.0) > 1.0e-12) {
		issues.emplace_back(
			IssueCode::NonAffineMatrix,
			"Coordinate reference transform matrix must have affine final row 0,0,0,1.");
	}
	if (std::isfinite(determinant(matrix)) &&
	    std::abs(determinant(matrix)) <= 1.0e-12) {
		issues.emplace_back(
			IssueCode::SingularMatrix,
			"Coordinate reference transform linear matrix must be invertible.");
	}
	if (transform.uncertaintyStatement().empty()) {
		issues.emplace_back(
			IssueCode::EmptyUncertaintyStatement,
			"Coordinate reference transform requires an uncertainty statement.");
	}
	if (transform.evidenceIdentifier().empty()) {
		issues.emplace_back(
			IssueCode::EmptyEvidenceIdentifier,
			"Coordinate reference transform requires an evidence identifier.");
	}
	if (!is_sha256(transform.evidenceSha256())) {
		issues.emplace_back(
			IssueCode::InvalidEvidenceSha256,
			"Coordinate reference transform evidence SHA-256 is malformed.");
	}
	return VegetationMeasuredCoordinateReferenceTransformValidationReport(
		std::move(issues));
}

VegetationMeasuredCoordinateReferenceTransformResult
VegetationMeasuredCoordinateReferenceTransformService::mapSourceToTarget(
	const VegetationMeasuredPoint3d &source_point,
	const VegetationMeasuredCoordinateReferenceTransform &transform) const
{
	const auto validation = validate(transform);
	if (!validation.valid()) {
		return rejected(
			transform.targetCoordinateSystem(), transform.targetCoordinateUnit(),
			"Coordinate reference transform is invalid.");
	}
	const VegetationMeasurementUnitNormalizationService normalization;
	const auto x = normalization.normalize(
		source_point.x(), transform.sourceCoordinateUnit(),
		VegetationMeasurementQuantityKind::LengthMetres);
	const auto y = normalization.normalize(
		source_point.y(), transform.sourceCoordinateUnit(),
		VegetationMeasurementQuantityKind::LengthMetres);
	const auto z = normalization.normalize(
		source_point.z(), transform.sourceCoordinateUnit(),
		VegetationMeasurementQuantityKind::LengthMetres);
	if (!x.succeeded() || !y.succeeded() || !z.succeeded()) {
		return rejected(
			transform.targetCoordinateSystem(), transform.targetCoordinateUnit(),
			"Source point cannot be normalized to metres.");
	}
	const auto &matrix = transform.sourceMetresToTargetMetres();
	return VegetationMeasuredCoordinateReferenceTransformResult(
		VegetationMeasuredPoint3d(
			matrix[0] * *x.normalizedValue() +
				matrix[1] * *y.normalizedValue() +
				matrix[2] * *z.normalizedValue() + matrix[3],
			matrix[4] * *x.normalizedValue() +
				matrix[5] * *y.normalizedValue() +
				matrix[6] * *z.normalizedValue() + matrix[7],
			matrix[8] * *x.normalizedValue() +
				matrix[9] * *y.normalizedValue() +
				matrix[10] * *z.normalizedValue() + matrix[11]),
		transform.targetCoordinateSystem(), transform.targetCoordinateUnit(),
		std::string());
}

VegetationMeasuredCoordinateReferenceTransformResult
VegetationMeasuredCoordinateReferenceTransformService::mapTargetToSource(
	const VegetationMeasuredPoint3d &target_point,
	const VegetationMeasuredCoordinateReferenceTransform &transform) const
{
	const auto validation = validate(transform);
	if (!validation.valid()) {
		return rejected(
			transform.sourceCoordinateSystem(), transform.sourceCoordinateUnit(),
			"Coordinate reference transform is invalid.");
	}
	if (!std::isfinite(target_point.x()) || !std::isfinite(target_point.y()) ||
	    !std::isfinite(target_point.z())) {
		return rejected(
			transform.sourceCoordinateSystem(), transform.sourceCoordinateUnit(),
			"Target point contains a non-finite coordinate.");
	}
	const auto &matrix = transform.sourceMetresToTargetMetres();
	const auto inverse = inverse_linear_matrix(matrix);
	const double translated_x = target_point.x() - matrix[3];
	const double translated_y = target_point.y() - matrix[7];
	const double translated_z = target_point.z() - matrix[11];
	const VegetationMeasuredPoint3d source_metres(
		inverse[0] * translated_x + inverse[1] * translated_y +
			inverse[2] * translated_z,
		inverse[3] * translated_x + inverse[4] * translated_y +
			inverse[5] * translated_z,
		inverse[6] * translated_x + inverse[7] * translated_y +
			inverse[8] * translated_z);
	return VegetationMeasuredCoordinateReferenceTransformResult(
		VegetationMeasuredPoint3d(
			metres_to_source_unit(
				source_metres.x(), transform.sourceCoordinateUnit()),
			metres_to_source_unit(
				source_metres.y(), transform.sourceCoordinateUnit()),
			metres_to_source_unit(
				source_metres.z(), transform.sourceCoordinateUnit())),
		transform.sourceCoordinateSystem(), transform.sourceCoordinateUnit(),
		std::string());
}
