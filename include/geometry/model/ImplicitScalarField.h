#pragma once

#include "geometry/model/ImplicitFieldBounds.h"

#include <glm/glm.hpp>

#include <cstdint>

class ImplicitScalarField
{
public:
	virtual ~ImplicitScalarField() = default;

	virtual double evaluateAt(const glm::dvec3 &position) const = 0;
	virtual ImplicitFieldBounds evaluationBounds() const = 0;
	virtual std::uint64_t deterministicHash() const = 0;
};
