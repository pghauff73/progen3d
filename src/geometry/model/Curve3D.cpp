#include "geometry/model/Curve3D.h"

#include "geometry/model/CurveArcLengthTable.h"
#include "geometry/service/CurveArcLengthService.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr std::size_t maximum_curve_control_points = 4096u;
constexpr std::size_t maximum_curve_samples = 16384u;

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

double clamp_parameter(double parameter)
{
	return std::clamp(parameter, 0.0, 1.0);
}

glm::dvec3 evaluate_bezier(
	const std::vector<glm::dvec3> &control_points,
	double parameter)
{
	std::vector<glm::dvec3> work = control_points;
	for (std::size_t level = 1u; level < work.size(); ++level) {
		for (std::size_t index = 0u; index + level < work.size(); ++index) {
			work[index] = glm::mix(work[index], work[index + 1u], parameter);
		}
	}
	return work.front();
}

std::vector<glm::dvec3> convert_points(const std::vector<glm::vec3> &points)
{
	std::vector<glm::dvec3> converted;
	converted.reserve(points.size());
	for (const glm::vec3 &point : points) converted.emplace_back(point);
	return converted;
}

glm::dvec3 evaluate_catmull_rom(
	const std::vector<glm::vec3> &control_points,
	double parameter,
	int derivative_order)
{
	const std::size_t segment_count = control_points.size() - 3u;
	const double scaled = clamp_parameter(parameter) *
	                      static_cast<double>(segment_count);
	const std::size_t segment = std::min(
		static_cast<std::size_t>(scaled), segment_count - 1u);
	const double local = segment + 1u == segment_count && parameter >= 1.0
		? 1.0
		: scaled - static_cast<double>(segment);
	const glm::dvec3 p0(control_points[segment]);
	const glm::dvec3 p1(control_points[segment + 1u]);
	const glm::dvec3 p2(control_points[segment + 2u]);
	const glm::dvec3 p3(control_points[segment + 3u]);
	const glm::dvec3 quadratic = 2.0 * p0 - 5.0 * p1 + 4.0 * p2 - p3;
	const glm::dvec3 cubic = -p0 + 3.0 * p1 - 3.0 * p2 + p3;
	if (derivative_order == 2) {
		const double scale = static_cast<double>(segment_count) *
		                     static_cast<double>(segment_count);
		return 0.5 * (2.0 * quadratic + 6.0 * cubic * local) * scale;
	}
	if (derivative_order == 1) {
		return 0.5 * ((-p0 + p2) + 2.0 * quadratic * local +
		              3.0 * cubic * local * local) *
		       static_cast<double>(segment_count);
	}
	return 0.5 * (2.0 * p1 + (-p0 + p2) * local +
	              quadratic * local * local + cubic * local * local * local);
}

std::vector<glm::dvec3> bezier_derivative_points(
	const std::vector<glm::dvec3> &points)
{
	std::vector<glm::dvec3> derivative_points;
	if (points.size() < 2u) return derivative_points;
	derivative_points.reserve(points.size() - 1u);
	const double degree = static_cast<double>(points.size() - 1u);
	for (std::size_t index = 0u; index + 1u < points.size(); ++index) {
		derivative_points.push_back((points[index + 1u] - points[index]) * degree);
	}
	return derivative_points;
}

} // namespace

bool Curve3D::isValid(std::string *diagnostic) const
{
	const std::size_t minimum_points = type_ == Curve3DType::CatmullRom ? 4u : 2u;
	if (control_points_.size() < minimum_points) {
		if (diagnostic != nullptr) {
			*diagnostic = "P3D-GEO-CURVE-001 Curve3D requires at least " +
			              std::to_string(minimum_points) + " control points.";
		}
		return false;
	}
	if (control_points_.size() > maximum_curve_control_points) {
		if (diagnostic != nullptr) {
			*diagnostic = "P3D-GEO-CURVE-005 Curve3D exceeds the control-point limit of " +
			              std::to_string(maximum_curve_control_points) + ".";
		}
		return false;
	}
	for (const glm::vec3 &point : control_points_) {
		if (!finite(point)) {
			if (diagnostic != nullptr) {
				*diagnostic = "P3D-GEO-CURVE-002 Curve3D control points must be finite.";
			}
			return false;
		}
	}
	double control_polygon_length = 0.0;
	for (std::size_t index = 1u; index < control_points_.size(); ++index) {
		control_polygon_length += glm::length(
			glm::dvec3(control_points_[index]) -
			glm::dvec3(control_points_[index - 1u]));
	}
	if (!std::isfinite(control_polygon_length) || control_polygon_length <= 1.0e-12) {
		if (diagnostic != nullptr) {
			*diagnostic = "P3D-GEO-CURVE-003 Curve3D requires nonzero geometric length.";
		}
		return false;
	}
	return true;
}

glm::dvec3 Curve3D::evaluatePosition(double parameter) const
{
	if (!isValid() || !std::isfinite(parameter)) return glm::dvec3(0.0);
	parameter = clamp_parameter(parameter);
	if (type_ == Curve3DType::Line) {
		return glm::mix(
			glm::dvec3(control_points_.front()),
			glm::dvec3(control_points_.back()), parameter);
	}
	if (type_ == Curve3DType::Polyline) {
		const std::size_t segment_count = control_points_.size() - 1u;
		const double scaled = parameter * static_cast<double>(segment_count);
		const std::size_t segment = std::min(
			static_cast<std::size_t>(scaled), segment_count - 1u);
		const double local = segment + 1u == segment_count && parameter >= 1.0
			? 1.0
			: scaled - static_cast<double>(segment);
		return glm::mix(
			glm::dvec3(control_points_[segment]),
			glm::dvec3(control_points_[segment + 1u]), local);
	}
	if (type_ == Curve3DType::Bezier) {
		return evaluate_bezier(convert_points(control_points_), parameter);
	}
	return evaluate_catmull_rom(control_points_, parameter, 0);
}

glm::dvec3 Curve3D::evaluateFirstDerivative(double parameter) const
{
	if (!isValid() || !std::isfinite(parameter)) return glm::dvec3(0.0);
	parameter = clamp_parameter(parameter);
	if (type_ == Curve3DType::Line) {
		return glm::dvec3(control_points_.back()) -
		       glm::dvec3(control_points_.front());
	}
	if (type_ == Curve3DType::Polyline) {
		const std::size_t segment_count = control_points_.size() - 1u;
		const std::size_t segment = std::min(
			static_cast<std::size_t>(parameter * static_cast<double>(segment_count)),
			segment_count - 1u);
		return (glm::dvec3(control_points_[segment + 1u]) -
		        glm::dvec3(control_points_[segment])) *
		       static_cast<double>(segment_count);
	}
	if (type_ == Curve3DType::Bezier) {
		return evaluate_bezier(
			bezier_derivative_points(convert_points(control_points_)), parameter);
	}
	return evaluate_catmull_rom(control_points_, parameter, 1);
}

glm::dvec3 Curve3D::evaluateSecondDerivative(double parameter) const
{
	if (!isValid() || !std::isfinite(parameter)) return glm::dvec3(0.0);
	parameter = clamp_parameter(parameter);
	if (type_ == Curve3DType::Line || type_ == Curve3DType::Polyline) {
		return glm::dvec3(0.0);
	}
	if (type_ == Curve3DType::Bezier) {
		const std::vector<glm::dvec3> first =
			bezier_derivative_points(convert_points(control_points_));
		const std::vector<glm::dvec3> second = bezier_derivative_points(first);
		return second.empty() ? glm::dvec3(0.0) :
		       evaluate_bezier(second, parameter);
	}
	return evaluate_catmull_rom(control_points_, parameter, 2);
}

glm::vec3 Curve3D::evaluate(float parameter) const
{
	return glm::vec3(evaluatePosition(static_cast<double>(parameter)));
}

glm::vec3 Curve3D::evaluateDerivative(float parameter) const
{
	return glm::vec3(evaluateFirstDerivative(static_cast<double>(parameter)));
}

glm::vec3 Curve3D::evaluateSecondDerivative(float parameter) const
{
	return glm::vec3(evaluateSecondDerivative(static_cast<double>(parameter)));
}

std::vector<glm::vec3> Curve3D::sample(std::size_t sample_count) const
{
	std::vector<glm::vec3> samples;
	if (!isValid() || sample_count < 2u || sample_count > maximum_curve_samples) {
		return samples;
	}
	samples.reserve(sample_count);
	for (std::size_t index = 0u; index < sample_count; ++index) {
		samples.push_back(evaluate(
			static_cast<float>(index) /
			static_cast<float>(sample_count - 1u)));
	}
	return samples;
}

std::shared_ptr<const CurveArcLengthTable> Curve3D::arcLengthTable(
	std::size_t sample_count) const
{
	if (cached_arc_length_table_ && cached_arc_length_sample_count_ == sample_count) {
		return cached_arc_length_table_;
	}
	cached_arc_length_table_ = CurveArcLengthService().build(
		*this, sample_count, maximum_curve_samples);
	cached_arc_length_sample_count_ = cached_arc_length_table_ ? sample_count : 0u;
	return cached_arc_length_table_;
}

double Curve3D::length(std::size_t arc_length_sample_count) const
{
	const auto table = arcLengthTable(arc_length_sample_count);
	return table ? table->totalLength() : 0.0;
}

double Curve3D::parameterAtDistance(
	double distance,
	std::size_t arc_length_sample_count) const
{
	if (!std::isfinite(distance)) return 0.0;
	const auto table = arcLengthTable(arc_length_sample_count);
	return table ? table->parameterAtDistance(distance) : 0.0;
}

glm::dvec3 Curve3D::evaluateByArcFraction(
	double arc_fraction,
	std::size_t arc_length_sample_count) const
{
	if (!std::isfinite(arc_fraction)) return glm::dvec3(0.0);
	const auto table = arcLengthTable(arc_length_sample_count);
	if (!table) return glm::dvec3(0.0);
	return evaluatePosition(table->parameterAtDistance(
		std::clamp(arc_fraction, 0.0, 1.0) * table->totalLength()));
}

std::vector<glm::dvec3> Curve3D::sampleByArcLength(
	std::size_t sample_count,
	std::size_t arc_length_sample_count) const
{
	std::vector<glm::dvec3> samples;
	if (sample_count < 2u || sample_count > maximum_curve_samples) return samples;
	const auto table = arcLengthTable(arc_length_sample_count);
	if (!table) return samples;
	samples.reserve(sample_count);
	for (std::size_t index = 0u; index < sample_count; ++index) {
		const double fraction = static_cast<double>(index) /
		                        static_cast<double>(sample_count - 1u);
		samples.push_back(evaluatePosition(
			table->parameterAtDistance(fraction * table->totalLength())));
	}
	return samples;
}
