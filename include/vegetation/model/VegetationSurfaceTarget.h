#pragma once

#include "geometry/model/AxisAlignedBounds.h"
#include "vegetation/model/VegetationSurfaceGeometryKind.h"

#include <string>
#include <utility>

class VegetationSurfaceTarget
{
public:
	VegetationSurfaceTarget(
		std::string identifier,
		VegetationSurfaceGeometryKind geometry_kind,
		AxisAlignedBounds bounds)
		: identifier_(std::move(identifier)),
		  geometry_kind_(geometry_kind),
		  bounds_(bounds)
	{
	}

	const std::string &identifier() const { return identifier_; }
	VegetationSurfaceGeometryKind geometryKind() const
	{
		return geometry_kind_;
	}
	const AxisAlignedBounds &bounds() const { return bounds_; }

private:
	std::string identifier_;
	VegetationSurfaceGeometryKind geometry_kind_ =
		VegetationSurfaceGeometryKind::AxisAlignedBox;
	AxisAlignedBounds bounds_;
};
