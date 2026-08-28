#pragma once

#include "grammar/model/ShapeDescriptorSyntax.h"

#include <memory>
#include <string>

class ShapeAliasExpansionService
{
public:
	std::shared_ptr<const ShapeDescriptorSyntax> expandAlias(
		const ShapeDescriptorSyntax &descriptor,
		std::string *diagnostic) const;
};
