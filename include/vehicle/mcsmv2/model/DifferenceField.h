#pragma once

#include "geometry/model/ImplicitScalarField.h"

#include <memory>

class DifferenceField final : public ImplicitScalarField
{
public:
	DifferenceField(
		std::shared_ptr<const ImplicitScalarField> source,
		std::shared_ptr<const ImplicitScalarField> subtract);

	double evaluateAt(const glm::dvec3 &position) const override;
	ImplicitFieldBounds evaluationBounds() const override;
	std::uint64_t deterministicHash() const override;

private:
	std::shared_ptr<const ImplicitScalarField> source_;
	std::shared_ptr<const ImplicitScalarField> subtract_;
};
