#pragma once

#include "grammar/model/CoaxialDriveSyntax.h"
#include "grammar/service/SpatialDeclarationParserTypes.h"
#include <memory>

class CoaxialDriveParser {
public:
	std::shared_ptr<const CoaxialDriveSyntax> parse(
		const std::vector<std::string> &tokens, std::size_t *index,
		const SpatialDeclarationParserTypes::ExpressionArgumentParser &expressions,
		std::string *diagnostic) const;
};
