#pragma once

#include "geometry/model/ImplicitScalarField.h"

#include <glm/glm.hpp>

class PackageBoundaryField final : public ImplicitScalarField
{
public:
	PackageBoundaryField(
		glm::dvec3 outward_normal,
		double offset,
		ImplicitFieldBounds bounds);

	double evaluateAt(const glm::dvec3 &position) const override;
	ImplicitFieldBounds evaluationBounds() const override { return bounds_; }
	std::uint64_t deterministicHash() const override;

private:
	glm::dvec3 outward_normal_{0.0, 0.0, 1.0};
	double offset_ = 0.0;
	ImplicitFieldBounds bounds_;
};
