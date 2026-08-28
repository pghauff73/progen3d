#include "vehicle/mcsmv2/service/VehicleSemanticSectionInterpolationService.h"

#include <cmath>
#include <stdexcept>
#include <vector>

namespace {

double signOf(double value)
{
	if (value > 0.0) return 1.0;
	if (value < 0.0) return -1.0;
	return 0.0;
}

double endpointDerivative(
	double first_interval,
	double second_interval,
	double first_slope,
	double second_slope)
{
	double derivative =
		((2.0 * first_interval + second_interval) * first_slope -
		 first_interval * second_slope) /
		(first_interval + second_interval);
	if (signOf(derivative) != signOf(first_slope)) return 0.0;
	if (signOf(first_slope) != signOf(second_slope) &&
	    std::fabs(derivative) > std::fabs(3.0 * first_slope)) {
		return 3.0 * first_slope;
	}
	return derivative;
}

} // namespace

ShapePreservingCubicInterpolation
VehicleSemanticSectionInterpolationService::createInterpolation(
	const std::vector<double> &coordinates,
	const std::vector<double> &values) const
{
	if (coordinates.size() < 2u || coordinates.size() != values.size()) {
		throw std::invalid_argument(
			"PCHIP interpolation requires equal coordinate and value arrays.");
	}
	std::vector<double> intervals(coordinates.size() - 1u, 0.0);
	std::vector<double> slopes(coordinates.size() - 1u, 0.0);
	for (std::size_t index = 0u; index < intervals.size(); ++index) {
		intervals[index] = coordinates[index + 1u] - coordinates[index];
		if (!(intervals[index] > 0.0)) {
			throw std::invalid_argument(
				"PCHIP interpolation coordinates must be strictly ordered.");
		}
		slopes[index] =
			(values[index + 1u] - values[index]) / intervals[index];
	}

	std::vector<double> derivatives(coordinates.size(), 0.0);
	if (coordinates.size() == 2u) {
		derivatives[0] = slopes[0];
		derivatives[1] = slopes[0];
		return ShapePreservingCubicInterpolation(
			coordinates, values, std::move(derivatives));
	}

	derivatives.front() = endpointDerivative(
		intervals[0], intervals[1], slopes[0], slopes[1]);
	derivatives.back() = endpointDerivative(
		intervals[intervals.size() - 1u],
		intervals[intervals.size() - 2u],
		slopes[slopes.size() - 1u],
		slopes[slopes.size() - 2u]);
	for (std::size_t index = 1u; index + 1u < coordinates.size(); ++index) {
		const double previous_slope = slopes[index - 1u];
		const double next_slope = slopes[index];
		if (previous_slope == 0.0 || next_slope == 0.0 ||
		    signOf(previous_slope) != signOf(next_slope)) {
			derivatives[index] = 0.0;
			continue;
		}
		const double previous_interval = intervals[index - 1u];
		const double next_interval = intervals[index];
		const double first_weight = 2.0 * next_interval + previous_interval;
		const double second_weight = next_interval + 2.0 * previous_interval;
		derivatives[index] = (first_weight + second_weight) /
			(first_weight / previous_slope + second_weight / next_slope);
	}
	return ShapePreservingCubicInterpolation(
		coordinates, values, std::move(derivatives));
}
