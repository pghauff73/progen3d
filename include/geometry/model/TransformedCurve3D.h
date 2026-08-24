#pragma once

#include "geometry/model/Curve3DEvaluator.h"

#include <glm/glm.hpp>

#include <memory>
#include <utility>

class TransformedCurve3D final : public Curve3DEvaluator
{
public:
	TransformedCurve3D(
		std::shared_ptr<const Curve3DEvaluator> source_curve,
		glm::dmat4 transformation)
		: source_curve_(std::move(source_curve)),
		  transformation_(transformation)
	{
	}

	const std::shared_ptr<const Curve3DEvaluator> &sourceCurve() const
	{
		return source_curve_;
	}
	const glm::dmat4 &transformation() const { return transformation_; }

	bool isValid(std::string *diagnostic = nullptr) const override;
	glm::dvec3 evaluatePosition(double parameter) const override;
	glm::dvec3 evaluateFirstDerivative(double parameter) const override;
	glm::dvec3 evaluateSecondDerivative(double parameter) const override;

private:
	std::shared_ptr<const Curve3DEvaluator> source_curve_;
	glm::dmat4 transformation_{1.0};
};
