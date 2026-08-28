#include "vehicle/mcsmv2/model/ShapePreservingCubicInterpolation.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

double ShapePreservingCubicInterpolation::evaluateAt(double coordinate) const
{
	if (coordinates_.size() < 2u || values_.size() != coordinates_.size() ||
	    derivatives_.size() != coordinates_.size()) {
		throw std::logic_error("Shape-preserving cubic interpolation is incomplete.");
	}
	const auto upper = std::upper_bound(
		coordinates_.begin(), coordinates_.end(), coordinate);
	std::size_t interval = 0u;
	if (upper == coordinates_.begin()) {
		interval = 0u;
	}
	else if (upper == coordinates_.end()) {
		interval = coordinates_.size() - 2u;
	}
	else {
		interval = static_cast<std::size_t>(
			std::distance(coordinates_.begin(), upper) - 1);
	}

	const double first_coordinate = coordinates_[interval];
	const double second_coordinate = coordinates_[interval + 1u];
	const double interval_length = second_coordinate - first_coordinate;
	const double parameter = (coordinate - first_coordinate) / interval_length;
	const double parameter_squared = parameter * parameter;
	const double parameter_cubed = parameter_squared * parameter;
	const double first_basis = 2.0 * parameter_cubed - 3.0 * parameter_squared + 1.0;
	const double first_derivative_basis =
		parameter_cubed - 2.0 * parameter_squared + parameter;
	const double second_basis = -2.0 * parameter_cubed + 3.0 * parameter_squared;
	const double second_derivative_basis = parameter_cubed - parameter_squared;
	return first_basis * values_[interval] +
	       first_derivative_basis * interval_length * derivatives_[interval] +
	       second_basis * values_[interval + 1u] +
	       second_derivative_basis * interval_length * derivatives_[interval + 1u];
}
