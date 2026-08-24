#include "geometry/model/CurveNetworkSurface.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

CurveNetworkSurface::CurveNetworkSurface(
	std::vector<Curve3D> constant_u_curves,
	std::vector<Curve3D> constant_v_curves,
	double intersection_tolerance)
	: constant_u_curves_(std::move(constant_u_curves)),
	  constant_v_curves_(std::move(constant_v_curves)),
	  intersection_tolerance_(intersection_tolerance)
{
}

bool CurveNetworkSurface::isValid(std::string *diagnostic) const
{
	if (constant_u_curves_.size() < 2u || constant_v_curves_.size() < 2u) {
		if (diagnostic != nullptr) {
			*diagnostic = "CurveNetworkSurface requires at least two curves in each direction.";
		}
		return false;
	}
	if (!std::isfinite(intersection_tolerance_) || intersection_tolerance_ <= 0.0) {
		if (diagnostic != nullptr) {
			*diagnostic = "CurveNetworkSurface intersection tolerance must be finite and positive.";
		}
		return false;
	}
	for (const Curve3D &curve : constant_u_curves_) {
		if (!curve.isValid(diagnostic)) return false;
	}
	for (const Curve3D &curve : constant_v_curves_) {
		if (!curve.isValid(diagnostic)) return false;
	}
	glm::dvec3 minimum(std::numeric_limits<double>::max());
	glm::dvec3 maximum(std::numeric_limits<double>::lowest());
	for (const Curve3D &curve : constant_u_curves_) {
		for (const glm::vec3 &point : curve.controlPoints()) {
			minimum = glm::min(minimum, glm::dvec3(point));
			maximum = glm::max(maximum, glm::dvec3(point));
		}
	}
	for (const Curve3D &curve : constant_v_curves_) {
		for (const glm::vec3 &point : curve.controlPoints()) {
			minimum = glm::min(minimum, glm::dvec3(point));
			maximum = glm::max(maximum, glm::dvec3(point));
		}
	}
	const double scale = std::max(1.0, glm::length(maximum - minimum));
	const double allowed_residual = intersection_tolerance_ * scale;
	for (std::size_t u_index = 0u; u_index < constant_u_curves_.size(); ++u_index) {
		const double u_parameter = static_cast<double>(u_index) /
			static_cast<double>(constant_u_curves_.size() - 1u);
		for (std::size_t v_index = 0u; v_index < constant_v_curves_.size(); ++v_index) {
			const double v_parameter = static_cast<double>(v_index) /
				static_cast<double>(constant_v_curves_.size() - 1u);
			const glm::dvec3 first = constant_u_curves_[u_index].evaluatePosition(v_parameter);
			const glm::dvec3 second = constant_v_curves_[v_index].evaluatePosition(u_parameter);
			if (glm::length(first - second) > allowed_residual) {
				if (diagnostic != nullptr) {
					*diagnostic = "CurveNetworkSurface curves do not intersect within tolerance at grid location (" +
						std::to_string(u_index) + ", " + std::to_string(v_index) + ").";
				}
				return false;
			}
		}
	}
	return true;
}

std::vector<double> CurveNetworkSurface::localLinearBasis(
	std::size_t count,
	double parameter)
{
	std::vector<double> weights(count, 0.0);
	const double clamped = std::max(0.0, std::min(1.0, parameter));
	const double scaled = clamped * static_cast<double>(count - 1u);
	const std::size_t first = std::min(
		count - 2u, static_cast<std::size_t>(std::floor(scaled)));
	const double local = scaled - static_cast<double>(first);
	weights[first] = 1.0 - local;
	weights[first + 1u] = local;
	return weights;
}

glm::dvec3 CurveNetworkSurface::intersectionPoint(
	std::size_t u_index,
	std::size_t v_index) const
{
	const double u_parameter = static_cast<double>(u_index) /
		static_cast<double>(constant_u_curves_.size() - 1u);
	const double v_parameter = static_cast<double>(v_index) /
		static_cast<double>(constant_v_curves_.size() - 1u);
	return 0.5 * (
		constant_u_curves_[u_index].evaluatePosition(v_parameter) +
		constant_v_curves_[v_index].evaluatePosition(u_parameter));
}

glm::dvec3 CurveNetworkSurface::evaluatePosition(double u, double v) const
{
	const double clamped_u = std::max(0.0, std::min(1.0, u));
	const double clamped_v = std::max(0.0, std::min(1.0, v));
	const std::vector<double> u_weights = localLinearBasis(
		constant_u_curves_.size(), clamped_u);
	const std::vector<double> v_weights = localLinearBasis(
		constant_v_curves_.size(), clamped_v);

	glm::dvec3 position(0.0);
	for (std::size_t u_index = 0u; u_index < constant_u_curves_.size(); ++u_index) {
		position += u_weights[u_index] *
			constant_u_curves_[u_index].evaluatePosition(clamped_v);
	}
	for (std::size_t v_index = 0u; v_index < constant_v_curves_.size(); ++v_index) {
		position += v_weights[v_index] *
			constant_v_curves_[v_index].evaluatePosition(clamped_u);
	}
	for (std::size_t u_index = 0u; u_index < constant_u_curves_.size(); ++u_index) {
		for (std::size_t v_index = 0u; v_index < constant_v_curves_.size(); ++v_index) {
			position -= u_weights[u_index] * v_weights[v_index] *
				intersectionPoint(u_index, v_index);
		}
	}
	return position;
}

const std::vector<Curve3D> &CurveNetworkSurface::constantUCurves() const
{
	return constant_u_curves_;
}

const std::vector<Curve3D> &CurveNetworkSurface::constantVCurves() const
{
	return constant_v_curves_;
}

double CurveNetworkSurface::intersectionTolerance() const
{
	return intersection_tolerance_;
}
