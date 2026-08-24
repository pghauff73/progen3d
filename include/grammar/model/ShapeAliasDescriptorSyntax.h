#pragma once

#include "grammar/model/ShapeDescriptorSyntax.h"

class ShapeAliasDescriptorSyntax : public StoredShapeDescriptorSyntax
{
public:
	ShapeAliasDescriptorSyntax(std::string alias_name,
	                           std::vector<ShapeOptionSyntax> options = {})
		: StoredShapeDescriptorSyntax(std::move(alias_name), std::move(options))
	{
	}
};
