#include "geometry/service/RigidTransformValidationService.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>

RigidTransformValidationReport RigidTransformValidationService::validate(
	const glm::dmat4 &transform,
	double tolerance) const
{
	bool finite = true;
	for (std::size_t column = 0u; column < 4u; ++column) {
		for (std::size_t row = 0u; row < 4u; ++row) {
			finite = finite && std::isfinite(transform[column][row]);
		}
	}
	const glm::dmat3 rotation(transform);
	const glm::dmat3 orthogonality = glm::transpose(rotation) * rotation;
	double maximum_orthogonality_error = 0.0;
	for (std::size_t column = 0u; column < 3u; ++column) {
		for (std::size_t row = 0u; row < 3u; ++row) {
			const double expected = column == row ? 1.0 : 0.0;
			maximum_orthogonality_error = std::max(
				maximum_orthogonality_error,
				std::abs(orthogonality[column][row] - expected));
		}
	}
	const double determinant = glm::determinant(rotation);
	const double homogeneous_row_error = std::max({
		std::abs(transform[0][3]), std::abs(transform[1][3]),
		std::abs(transform[2][3]), std::abs(transform[3][3] - 1.0)});
	const bool passed = finite && maximum_orthogonality_error <= tolerance &&
		std::abs(determinant - 1.0) <= tolerance && homogeneous_row_error <= tolerance;
	return RigidTransformValidationReport(
		finite, maximum_orthogonality_error, determinant, homogeneous_row_error,
		passed);
}
