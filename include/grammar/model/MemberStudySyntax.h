#pragma once

#include "grammar/model/GeometryExpression.h"
#include <array>
#include <sstream>

struct MemberStudySyntax {
	std::string pin, roller, material;
	GeometryExpression metresPerUnit{"1"}, modulus{"0"}, density{"0"};
	std::array<GeometryExpression, 3> section{GeometryExpression("0"), GeometryExpression("0"), GeometryExpression("0")};
	std::array<GeometryExpression, 3> load = section;

	std::string canonicalText() const
	{
		std::ostringstream out;
		out << "PinRollerMember(supports(" << pin << " " << roller << ") metresPerUnit("
		    << metresPerUnit.sourceText() << ") material(" << material << " "
		    << modulus.sourceText() << " " << density.sourceText() << ") section("
		    << section[0].sourceText() << " " << section[1].sourceText() << " " << section[2].sourceText()
		    << ") load(" << load[0].sourceText() << " " << load[1].sourceText() << " " << load[2].sourceText() << "))";
		return out.str();
	}
};
