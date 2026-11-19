#pragma once

#include "grammar/model/GeometryExpression.h"
#include <array>

struct CoaxialDriveSyntax {
	using Pair = std::array<GeometryExpression, 2>;
	Pair hoop{GeometryExpression("0"), GeometryExpression("0")};
	Pair motion = hoop;
	Pair motor = hoop;
	std::array<GeometryExpression, 3> transmission{
		GeometryExpression("1"), GeometryExpression("1"), GeometryExpression("1")};
	std::string canonicalText() const;
};
