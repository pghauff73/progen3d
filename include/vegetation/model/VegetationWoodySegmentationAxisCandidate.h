#pragma once

#include "vegetation/model/VegetationMeasuredPoint3d.h"

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

class VegetationWoodySegmentationAxisCandidate
{
public:
	VegetationWoodySegmentationAxisCandidate(
		std::string axis_identifier,
		std::string parent_axis_identifier,
		std::size_t branch_order,
		std::vector<std::string> owned_cover_identifiers,
		std::vector<VegetationMeasuredPoint3d> centreline_points_metres,
		double axial_station_spacing_metres)
		: axis_identifier_(std::move(axis_identifier)),
		  parent_axis_identifier_(std::move(parent_axis_identifier)),
		  branch_order_(branch_order),
		  owned_cover_identifiers_(std::move(owned_cover_identifiers)),
		  centreline_points_metres_(std::move(centreline_points_metres)),
		  axial_station_spacing_metres_(axial_station_spacing_metres)
	{
	}

	const std::string &axisIdentifier() const { return axis_identifier_; }
	const std::string &parentAxisIdentifier() const
	{
		return parent_axis_identifier_;
	}
	std::size_t branchOrder() const { return branch_order_; }
	const std::vector<std::string> &ownedCoverIdentifiers() const
	{
		return owned_cover_identifiers_;
	}
	const std::vector<VegetationMeasuredPoint3d> &centrelinePointsMetres() const
	{
		return centreline_points_metres_;
	}
	double axialStationSpacingMetres() const
	{
		return axial_station_spacing_metres_;
	}

private:
	std::string axis_identifier_;
	std::string parent_axis_identifier_;
	std::size_t branch_order_ = 0u;
	std::vector<std::string> owned_cover_identifiers_;
	std::vector<VegetationMeasuredPoint3d> centreline_points_metres_;
	double axial_station_spacing_metres_ = 0.0;
};
