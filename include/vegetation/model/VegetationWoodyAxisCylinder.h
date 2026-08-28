#pragma once

#include "vegetation/model/VegetationMeasuredPoint3d.h"

#include <cstddef>
#include <string>
#include <utility>

class VegetationWoodyAxisCylinder
{
public:
	VegetationWoodyAxisCylinder(
		std::string cylinder_identifier,
		std::size_t cylinder_index,
		VegetationMeasuredPoint3d start_point_metres,
		VegetationMeasuredPoint3d end_point_metres,
		double radius_metres,
		std::size_t supporting_point_count)
		: cylinder_identifier_(std::move(cylinder_identifier)),
		  cylinder_index_(cylinder_index),
		  start_point_metres_(start_point_metres),
		  end_point_metres_(end_point_metres),
		  radius_metres_(radius_metres),
		  supporting_point_count_(supporting_point_count)
	{
	}

	const std::string &cylinderIdentifier() const
	{
		return cylinder_identifier_;
	}
	std::size_t cylinderIndex() const { return cylinder_index_; }
	const VegetationMeasuredPoint3d &startPointMetres() const
	{
		return start_point_metres_;
	}
	const VegetationMeasuredPoint3d &endPointMetres() const
	{
		return end_point_metres_;
	}
	double radiusMetres() const { return radius_metres_; }
	std::size_t supportingPointCount() const
	{
		return supporting_point_count_;
	}

private:
	std::string cylinder_identifier_;
	std::size_t cylinder_index_ = 0u;
	VegetationMeasuredPoint3d start_point_metres_{0.0, 0.0, 0.0};
	VegetationMeasuredPoint3d end_point_metres_{0.0, 0.0, 0.0};
	double radius_metres_ = 0.0;
	std::size_t supporting_point_count_ = 0u;
};
