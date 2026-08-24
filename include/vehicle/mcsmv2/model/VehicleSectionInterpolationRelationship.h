#pragma once

#include <cstddef>

class VehicleSectionInterpolationRelationship
{
public:
	VehicleSectionInterpolationRelationship(
		double source_x,
		std::size_t first_station_index,
		std::size_t second_station_index,
		double interpolation_parameter)
		: source_x_(source_x),
		  first_station_index_(first_station_index),
		  second_station_index_(second_station_index),
		  interpolation_parameter_(interpolation_parameter)
	{
	}

	double sourceX() const { return source_x_; }
	std::size_t firstStationIndex() const { return first_station_index_; }
	std::size_t secondStationIndex() const { return second_station_index_; }
	double interpolationParameter() const { return interpolation_parameter_; }

private:
	double source_x_ = 0.0;
	std::size_t first_station_index_ = 0u;
	std::size_t second_station_index_ = 0u;
	double interpolation_parameter_ = 0.0;
};
