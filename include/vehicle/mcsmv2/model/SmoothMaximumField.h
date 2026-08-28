#pragma once

#include "geometry/model/ImplicitScalarField.h"

#include <memory>

class SmoothMaximumField final : public ImplicitScalarField
{
public:
	SmoothMaximumField(
		std::shared_ptr<const ImplicitScalarField> first,
		std::shared_ptr<const ImplicitScalarField> second,
		double sharpness);

	double evaluateAt(const glm::dvec3 &position) const override;
	ImplicitFieldBounds evaluationBounds() const override;
	std::uint64_t deterministicHash() const override;

private:
	std::shared_ptr<const ImplicitScalarField> first_;
	std::shared_ptr<const ImplicitScalarField> second_;
	double sharpness_ = 1.0;
};
