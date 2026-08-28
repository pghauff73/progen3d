#include "geometry/model/Surface3D.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr double derivative_step = 1.0e-5;

glm::dvec3 finite_difference(
	const Surface3D &surface,
	double u,
	double v,
	bool along_u)
{
	const double parameter = along_u ? u : v;
	const double lower = std::max(0.0, parameter - derivative_step);
	const double upper = std::min(1.0, parameter + derivative_step);
	if (upper <= lower) return glm::dvec3(0.0);
	const glm::dvec3 first = along_u
		? surface.evaluatePosition(lower, v)
		: surface.evaluatePosition(u, lower);
	const glm::dvec3 second = along_u
		? surface.evaluatePosition(upper, v)
		: surface.evaluatePosition(u, upper);
	return (second - first) / (upper - lower);
}

}

glm::dvec3 Surface3D::evaluatePartialDerivativeU(double u, double v) const
{
	return finite_difference(*this, u, v, true);
}

glm::dvec3 Surface3D::evaluatePartialDerivativeV(double u, double v) const
{
	return finite_difference(*this, u, v, false);
}

glm::dvec3 Surface3D::evaluateNormal(double u, double v) const
{
	const glm::dvec3 cross = glm::cross(
		evaluatePartialDerivativeU(u, v),
		evaluatePartialDerivativeV(u, v));
	const double length_squared = glm::dot(cross, cross);
	return length_squared <= 1.0e-24
		? glm::dvec3(0.0)
		: cross / std::sqrt(length_squared);
}
