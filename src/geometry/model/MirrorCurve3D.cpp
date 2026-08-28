#include "geometry/model/MirrorCurve3D.h"

#include <utility>

MirrorCurve3D::MirrorCurve3D(
	std::shared_ptr<const Curve3DEvaluator> source_curve,
	MirrorCurvePlaneAxis plane_axis,
	double plane_offset)
	: plane_axis_(plane_axis),
	  plane_offset_(plane_offset),
	  transformed_curve_(
		  std::move(source_curve),
		  createTransformation(plane_axis, plane_offset))
{
}

glm::dmat4 MirrorCurve3D::createTransformation(
	MirrorCurvePlaneAxis plane_axis,
	double plane_offset)
{
	glm::dmat4 transformation(1.0);
	int axis_index = 0;
	if (plane_axis == MirrorCurvePlaneAxis::Y) axis_index = 1;
	else if (plane_axis == MirrorCurvePlaneAxis::Z) axis_index = 2;
	transformation[axis_index][axis_index] = -1.0;
	transformation[3][axis_index] = 2.0 * plane_offset;
	return transformation;
}
