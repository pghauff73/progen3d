#pragma once

#include <cstddef>
#include <string>
#include <utility>

class VegetationWoodySegmentationParameterSet
{
public:
	VegetationWoodySegmentationParameterSet(
		std::string parameter_identifier,
		double cover_cell_size_metres,
		double neighbour_radius_multiplier,
		std::size_t maximum_cover_index_delta,
		std::size_t minimum_cover_point_count,
		std::size_t maximum_cover_set_count,
		std::size_t maximum_axis_count,
		double minimum_continuation_cosine,
		double minimum_assignment_confidence,
		std::size_t minimum_cylinders_per_axis,
		double minimum_axial_station_spacing_metres)
		: parameter_identifier_(std::move(parameter_identifier)),
		  cover_cell_size_metres_(cover_cell_size_metres),
		  neighbour_radius_multiplier_(neighbour_radius_multiplier),
		  maximum_cover_index_delta_(maximum_cover_index_delta),
		  minimum_cover_point_count_(minimum_cover_point_count),
		  maximum_cover_set_count_(maximum_cover_set_count),
		  maximum_axis_count_(maximum_axis_count),
		  minimum_continuation_cosine_(minimum_continuation_cosine),
		  minimum_assignment_confidence_(minimum_assignment_confidence),
		  minimum_cylinders_per_axis_(minimum_cylinders_per_axis),
		  minimum_axial_station_spacing_metres_(
			  minimum_axial_station_spacing_metres)
	{
	}

	const std::string &parameterIdentifier() const
	{
		return parameter_identifier_;
	}
	double coverCellSizeMetres() const { return cover_cell_size_metres_; }
	double neighbourRadiusMultiplier() const
	{
		return neighbour_radius_multiplier_;
	}
	std::size_t maximumCoverIndexDelta() const
	{
		return maximum_cover_index_delta_;
	}
	std::size_t minimumCoverPointCount() const
	{
		return minimum_cover_point_count_;
	}
	std::size_t maximumCoverSetCount() const
	{
		return maximum_cover_set_count_;
	}
	std::size_t maximumAxisCount() const { return maximum_axis_count_; }
	double minimumContinuationCosine() const
	{
		return minimum_continuation_cosine_;
	}
	double minimumAssignmentConfidence() const
	{
		return minimum_assignment_confidence_;
	}
	std::size_t minimumCylindersPerAxis() const
	{
		return minimum_cylinders_per_axis_;
	}
	double minimumAxialStationSpacingMetres() const
	{
		return minimum_axial_station_spacing_metres_;
	}

private:
	std::string parameter_identifier_;
	double cover_cell_size_metres_ = 0.0;
	double neighbour_radius_multiplier_ = 0.0;
	std::size_t maximum_cover_index_delta_ = 0u;
	std::size_t minimum_cover_point_count_ = 0u;
	std::size_t maximum_cover_set_count_ = 0u;
	std::size_t maximum_axis_count_ = 0u;
	double minimum_continuation_cosine_ = 0.0;
	double minimum_assignment_confidence_ = 0.0;
	std::size_t minimum_cylinders_per_axis_ = 0u;
	double minimum_axial_station_spacing_metres_ = 0.0;
};
