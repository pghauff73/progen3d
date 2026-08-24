#pragma once

#include <cmath>
#include <stdexcept>

class VehicleSurfaceCoordinate
{
public:
	VehicleSurfaceCoordinate(double longitudinal_parameter, double periodic_parameter)
		: longitudinal_parameter_(longitudinal_parameter),
		  periodic_parameter_(periodic_parameter)
	{
		if (!std::isfinite(longitudinal_parameter_) ||
		    !std::isfinite(periodic_parameter_) ||
		    longitudinal_parameter_ < 0.0 || longitudinal_parameter_ > 1.0 ||
		    periodic_parameter_ < 0.0 || periodic_parameter_ > 1.0) {
			throw std::invalid_argument(
				"Vehicle surface coordinates must be finite normalized values.");
		}
	}

	double longitudinalParameter() const { return longitudinal_parameter_; }
	double periodicParameter() const { return periodic_parameter_; }

private:
	double longitudinal_parameter_ = 0.0;
	double periodic_parameter_ = 0.0;
};
