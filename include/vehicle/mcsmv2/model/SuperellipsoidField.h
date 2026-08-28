#pragma once

#include "geometry/model/ImplicitScalarField.h"

#include <glm/glm.hpp>

class SuperellipsoidField final : public ImplicitScalarField
{
public:
	SuperellipsoidField(
		glm::dvec3 center,
		glm::dvec3 radii,
		double exponent);

	double evaluateAt(const glm::dvec3 &position) const override;
	ImplicitFieldBounds evaluationBounds() const override;
	std::uint64_t deterministicHash() const override;

private:
	glm::dvec3 center_{0.0};
	glm::dvec3 radii_{1.0};
	double exponent_ = 2.0;
};
