#pragma once

#include "vegetation/model/VegetationMeasuredPoint3d.h"

#include <cstddef>
#include <string>
#include <utility>

class VegetationWoodyAxisRadiusStation
{
public:
	VegetationWoodyAxisRadiusStation(
		std::string station_identifier,
		std::size_t station_index,
		double axial_projection_metres,
		VegetationMeasuredPoint3d centre_point_metres,
		double radius_metres,
		std::size_t supporting_point_count)
		: station_identifier_(std::move(station_identifier)),
		  station_index_(station_index),
		  axial_projection_metres_(axial_projection_metres),
		  centre_point_metres_(centre_point_metres),
		  radius_metres_(radius_metres),
		  supporting_point_count_(supporting_point_count)
	{
	}

	const std::string &stationIdentifier() const
	{
		return station_identifier_;
	}
	std::size_t stationIndex() const { return station_index_; }
	double axialProjectionMetres() const
	{
		return axial_projection_metres_;
	}
	const VegetationMeasuredPoint3d &centrePointMetres() const
	{
		return centre_point_metres_;
	}
	double radiusMetres() const { return radius_metres_; }
	std::size_t supportingPointCount() const
	{
		return supporting_point_count_;
	}

private:
	std::string station_identifier_;
	std::size_t station_index_ = 0u;
	double axial_projection_metres_ = 0.0;
	VegetationMeasuredPoint3d centre_point_metres_{0.0, 0.0, 0.0};
	double radius_metres_ = 0.0;
	std::size_t supporting_point_count_ = 0u;
};
