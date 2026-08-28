#pragma once

#include "vegetation/model/VegetationMeasuredPoint3d.h"

#include <cstddef>
#include <string>
#include <utility>

class VegetationRootArchitectureGraphSegmentRecord
{
public:
	VegetationRootArchitectureGraphSegmentRecord(
		std::string segment_identifier,
		std::string parent_segment_identifier,
		std::size_t root_order,
		VegetationMeasuredPoint3d start_point_metres,
		VegetationMeasuredPoint3d end_point_metres,
		double radius_metres)
		: segment_identifier_(std::move(segment_identifier)),
		  parent_segment_identifier_(std::move(parent_segment_identifier)),
		  root_order_(root_order),
		  start_point_metres_(start_point_metres),
		  end_point_metres_(end_point_metres),
		  radius_metres_(radius_metres)
	{
	}

	const std::string &segmentIdentifier() const { return segment_identifier_; }
	const std::string &parentSegmentIdentifier() const
	{
		return parent_segment_identifier_;
	}
	std::size_t rootOrder() const { return root_order_; }
	const VegetationMeasuredPoint3d &startPointMetres() const
	{
		return start_point_metres_;
	}
	const VegetationMeasuredPoint3d &endPointMetres() const
	{
		return end_point_metres_;
	}
	double radiusMetres() const { return radius_metres_; }

private:
	std::string segment_identifier_;
	std::string parent_segment_identifier_;
	std::size_t root_order_ = 0u;
	VegetationMeasuredPoint3d start_point_metres_{0.0, 0.0, 0.0};
	VegetationMeasuredPoint3d end_point_metres_{0.0, 0.0, 0.0};
	double radius_metres_ = 0.0;
};
