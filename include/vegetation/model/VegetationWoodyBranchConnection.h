#pragma once

#include "vegetation/model/VegetationMeasuredPoint3d.h"

#include <string>
#include <utility>

class VegetationWoodyBranchConnection
{
public:
	VegetationWoodyBranchConnection(
		std::string parent_axis_identifier,
		std::string child_axis_identifier,
		std::string parent_cylinder_identifier,
		std::string child_root_cylinder_identifier,
		VegetationMeasuredPoint3d connection_point_metres,
		double attachment_surface_gap_metres)
		: parent_axis_identifier_(std::move(parent_axis_identifier)),
		  child_axis_identifier_(std::move(child_axis_identifier)),
		  parent_cylinder_identifier_(
			  std::move(parent_cylinder_identifier)),
		  child_root_cylinder_identifier_(
			  std::move(child_root_cylinder_identifier)),
		  connection_point_metres_(connection_point_metres),
		  attachment_surface_gap_metres_(attachment_surface_gap_metres)
	{
	}

	const std::string &parentAxisIdentifier() const
	{
		return parent_axis_identifier_;
	}
	const std::string &childAxisIdentifier() const
	{
		return child_axis_identifier_;
	}
	const std::string &parentCylinderIdentifier() const
	{
		return parent_cylinder_identifier_;
	}
	const std::string &childRootCylinderIdentifier() const
	{
		return child_root_cylinder_identifier_;
	}
	const VegetationMeasuredPoint3d &connectionPointMetres() const
	{
		return connection_point_metres_;
	}
	double attachmentSurfaceGapMetres() const
	{
		return attachment_surface_gap_metres_;
	}

private:
	std::string parent_axis_identifier_;
	std::string child_axis_identifier_;
	std::string parent_cylinder_identifier_;
	std::string child_root_cylinder_identifier_;
	VegetationMeasuredPoint3d connection_point_metres_{0.0, 0.0, 0.0};
	double attachment_surface_gap_metres_ = 0.0;
};
