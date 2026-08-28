#pragma once

#include <cstddef>
#include <string>
#include <utility>

class VegetationWoodyAxisSegmentDefinition
{
public:
	VegetationWoodyAxisSegmentDefinition(
		std::string axis_identifier,
		std::string parent_axis_identifier,
		std::size_t branch_order,
		double axial_station_spacing_metres)
		: axis_identifier_(std::move(axis_identifier)),
		  parent_axis_identifier_(std::move(parent_axis_identifier)),
		  branch_order_(branch_order),
		  axial_station_spacing_metres_(axial_station_spacing_metres)
	{
	}

	const std::string &axisIdentifier() const { return axis_identifier_; }
	const std::string &parentAxisIdentifier() const
	{
		return parent_axis_identifier_;
	}
	std::size_t branchOrder() const { return branch_order_; }
	double axialStationSpacingMetres() const
	{
		return axial_station_spacing_metres_;
	}
	bool isRootAxis() const { return parent_axis_identifier_.empty(); }

private:
	std::string axis_identifier_;
	std::string parent_axis_identifier_;
	std::size_t branch_order_ = 0u;
	double axial_station_spacing_metres_ = 0.0;
};
