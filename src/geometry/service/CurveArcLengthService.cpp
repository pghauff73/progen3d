#include "geometry/service/CurveArcLengthService.h"

#include "geometry/model/Curve3DEvaluator.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <utility>
#include <vector>

std::shared_ptr<const CurveArcLengthTable> CurveArcLengthService::build(
	const Curve3DEvaluator &curve,
	std::size_t sample_count,
	std::size_t maximum_sample_count,
	std::string *diagnostic) const
{
	std::string curve_diagnostic;
	if (!curve.isValid(&curve_diagnostic)) {
		if (diagnostic != nullptr) *diagnostic = std::move(curve_diagnostic);
		return {};
	}
	if (sample_count < 2u || sample_count > maximum_sample_count) {
		if (diagnostic != nullptr) {
			*diagnostic = "P3D-GEO-CURVE-005 arc-length sample count must be between 2 and " +
			              std::to_string(maximum_sample_count) + ".";
		}
		return {};
	}

	std::vector<CurveArcLengthSample> samples;
	samples.reserve(sample_count);
	glm::dvec3 previous = curve.evaluatePosition(0.0);
	if (!std::isfinite(previous.x) || !std::isfinite(previous.y) ||
	    !std::isfinite(previous.z)) {
		if (diagnostic != nullptr) {
			*diagnostic = "P3D-GEO-CURVE-004 curve evaluation produced a nonfinite position.";
		}
		return {};
	}
	double distance = 0.0;
	samples.emplace_back(0.0, 0.0);
	for (std::size_t index = 1u; index < sample_count; ++index) {
		const double parameter = static_cast<double>(index) /
		                         static_cast<double>(sample_count - 1u);
		const glm::dvec3 position = curve.evaluatePosition(parameter);
		if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
		    !std::isfinite(position.z)) {
			if (diagnostic != nullptr) {
				*diagnostic = "P3D-GEO-CURVE-004 curve evaluation produced a nonfinite position.";
			}
			return {};
		}
		distance += glm::length(position - previous);
		samples.emplace_back(parameter, distance);
		previous = position;
	}
	if (!std::isfinite(distance) || distance <= 1.0e-12) {
		if (diagnostic != nullptr) {
			*diagnostic = "P3D-GEO-CURVE-003 curve has zero measurable length.";
		}
		return {};
	}
	return std::make_shared<const CurveArcLengthTable>(std::move(samples));
}
