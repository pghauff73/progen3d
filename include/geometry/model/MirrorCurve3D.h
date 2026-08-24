#pragma once

#include "geometry/model/TransformedCurve3D.h"

enum class MirrorCurvePlaneAxis
{
	X,
	Y,
	Z
};

class MirrorCurve3D final : public Curve3DEvaluator
{
public:
	MirrorCurve3D(
		std::shared_ptr<const Curve3DEvaluator> source_curve,
		MirrorCurvePlaneAxis plane_axis,
		double plane_offset = 0.0);

	const TransformedCurve3D &transformedCurve() const
	{
		return transformed_curve_;
	}
	MirrorCurvePlaneAxis planeAxis() const { return plane_axis_; }
	double planeOffset() const { return plane_offset_; }

	bool isValid(std::string *diagnostic = nullptr) const override
	{
		return transformed_curve_.isValid(diagnostic);
	}
	glm::dvec3 evaluatePosition(double parameter) const override
	{
		return transformed_curve_.evaluatePosition(parameter);
	}
	glm::dvec3 evaluateFirstDerivative(double parameter) const override
	{
		return transformed_curve_.evaluateFirstDerivative(parameter);
	}
	glm::dvec3 evaluateSecondDerivative(double parameter) const override
	{
		return transformed_curve_.evaluateSecondDerivative(parameter);
	}

private:
	static glm::dmat4 createTransformation(
		MirrorCurvePlaneAxis plane_axis,
		double plane_offset);

	MirrorCurvePlaneAxis plane_axis_ = MirrorCurvePlaneAxis::X;
	double plane_offset_ = 0.0;
	TransformedCurve3D transformed_curve_;
};
