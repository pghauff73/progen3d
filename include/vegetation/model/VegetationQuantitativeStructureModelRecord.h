#pragma once

#include "vegetation/model/VegetationMeasuredPoint3d.h"

#include <cstddef>
#include <string>
#include <utility>

class VegetationQuantitativeStructureModelCylinderRecord
{
public:
	VegetationQuantitativeStructureModelCylinderRecord(
		std::string cylinder_identifier,
		std::string parent_cylinder_identifier,
		std::size_t branch_order,
		VegetationMeasuredPoint3d start_point_metres,
		VegetationMeasuredPoint3d end_point_metres,
		double radius_metres)
		: cylinder_identifier_(std::move(cylinder_identifier)),
		  parent_cylinder_identifier_(std::move(parent_cylinder_identifier)),
		  branch_order_(branch_order),
		  start_point_metres_(start_point_metres),
		  end_point_metres_(end_point_metres),
		  radius_metres_(radius_metres)
	{
	}

	const std::string &cylinderIdentifier() const { return cylinder_identifier_; }
	const std::string &parentCylinderIdentifier() const
	{
		return parent_cylinder_identifier_;
	}
	std::size_t branchOrder() const { return branch_order_; }
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
	std::string cylinder_identifier_;
	std::string parent_cylinder_identifier_;
	std::size_t branch_order_ = 0u;
	VegetationMeasuredPoint3d start_point_metres_{0.0, 0.0, 0.0};
	VegetationMeasuredPoint3d end_point_metres_{0.0, 0.0, 0.0};
	double radius_metres_ = 0.0;
};
