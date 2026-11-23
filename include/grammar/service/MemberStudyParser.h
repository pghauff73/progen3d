#pragma once

#include "grammar/model/MemberStudySyntax.h"
#include "grammar/service/SpatialDeclarationParserTypes.h"
#include <memory>

class MemberStudyParser {
public:
	std::shared_ptr<const MemberStudySyntax> parse(
		const std::vector<std::string> &tokens, std::size_t *index,
		const SpatialDeclarationParserTypes::ExpressionArgumentParser &expressions,
		std::string *diagnostic) const;
};
