#pragma once

#include "vegetation/model/VegetationMeasuredPoint3d.h"
#include "vegetation/model/VegetationWoodyAxisRadiusStation.h"
#include "vegetation/model/VegetationWoodyBranchCylinder.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class VegetationWoodyBranchAxis
{
public:
	VegetationWoodyBranchAxis(
		std::string axis_identifier,
		std::string parent_axis_identifier,
		std::size_t branch_order,
		VegetationMeasuredPoint3d axis_centroid_metres,
		VegetationMeasuredPoint3d axis_direction,
		std::vector<VegetationWoodyAxisRadiusStation> radius_stations,
		std::vector<VegetationWoodyBranchCylinder> cylinders)
		: axis_identifier_(std::move(axis_identifier)),
		  parent_axis_identifier_(std::move(parent_axis_identifier)),
		  branch_order_(branch_order),
		  axis_centroid_metres_(axis_centroid_metres),
		  axis_direction_(axis_direction),
		  radius_stations_(std::move(radius_stations)),
		  cylinders_(std::move(cylinders))
	{
	}

	const std::string &axisIdentifier() const { return axis_identifier_; }
	const std::string &parentAxisIdentifier() const
	{
		return parent_axis_identifier_;
	}
	std::size_t branchOrder() const { return branch_order_; }
	const VegetationMeasuredPoint3d &axisCentroidMetres() const
	{
		return axis_centroid_metres_;
	}
	const VegetationMeasuredPoint3d &axisDirection() const
	{
		return axis_direction_;
	}
	const std::vector<VegetationWoodyAxisRadiusStation> &radiusStations() const
	{
		return radius_stations_;
	}
	const std::vector<VegetationWoodyBranchCylinder> &cylinders() const
	{
		return cylinders_;
	}

private:
	std::string axis_identifier_;
	std::string parent_axis_identifier_;
	std::size_t branch_order_ = 0u;
	VegetationMeasuredPoint3d axis_centroid_metres_{0.0, 0.0, 0.0};
	VegetationMeasuredPoint3d axis_direction_{0.0, 0.0, 1.0};
	std::vector<VegetationWoodyAxisRadiusStation> radius_stations_;
	std::vector<VegetationWoodyBranchCylinder> cylinders_;
};
