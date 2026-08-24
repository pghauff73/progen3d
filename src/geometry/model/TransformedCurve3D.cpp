#include "geometry/model/TransformedCurve3D.h"

#include <cmath>

namespace {

bool finite(const glm::dmat4 &matrix)
{
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			if (!std::isfinite(matrix[column][row])) return false;
		}
	}
	return true;
}

}

bool TransformedCurve3D::isValid(std::string *diagnostic) const
{
	if (!source_curve_) {
		if (diagnostic != nullptr) {
			*diagnostic = "P3D-GEO-CURVE-004 transformed curve requires a source curve.";
		}
		return false;
	}
	if (!finite(transformation_)) {
		if (diagnostic != nullptr) {
			*diagnostic = "P3D-GEO-CURVE-004 transformed curve matrix must be finite.";
		}
		return false;
	}
	return source_curve_->isValid(diagnostic);
}

glm::dvec3 TransformedCurve3D::evaluatePosition(double parameter) const
{
	if (!isValid()) return glm::dvec3(0.0);
	return glm::dvec3(
		transformation_ * glm::dvec4(source_curve_->evaluatePosition(parameter), 1.0));
}

glm::dvec3 TransformedCurve3D::evaluateFirstDerivative(double parameter) const
{
	if (!isValid()) return glm::dvec3(0.0);
	return glm::dmat3(transformation_) *
	       source_curve_->evaluateFirstDerivative(parameter);
}

glm::dvec3 TransformedCurve3D::evaluateSecondDerivative(double parameter) const
{
	if (!isValid()) return glm::dvec3(0.0);
	return glm::dmat3(transformation_) *
	       source_curve_->evaluateSecondDerivative(parameter);
}
