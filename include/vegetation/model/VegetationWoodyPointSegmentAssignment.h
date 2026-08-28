#pragma once

#include <cstddef>
#include <string>
#include <utility>

class VegetationWoodyPointSegmentAssignment
{
public:
	VegetationWoodyPointSegmentAssignment(
		std::size_t source_point_index,
		std::string axis_identifier,
		double assignment_confidence)
		: source_point_index_(source_point_index),
		  axis_identifier_(std::move(axis_identifier)),
		  assignment_confidence_(assignment_confidence)
	{
	}

	std::size_t sourcePointIndex() const { return source_point_index_; }
	const std::string &axisIdentifier() const { return axis_identifier_; }
	double assignmentConfidence() const { return assignment_confidence_; }

private:
	std::size_t source_point_index_ = 0u;
	std::string axis_identifier_;
	double assignment_confidence_ = 0.0;
};
