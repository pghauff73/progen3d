#pragma once

#include "grammar/model/ShapeDescriptorSyntax.h"

#include <utility>

class BranchJunctionShapeDescriptorSyntax final : public StoredShapeDescriptorSyntax
{
public:
	explicit BranchJunctionShapeDescriptorSyntax(
		std::vector<ShapeOptionSyntax> options)
		: StoredShapeDescriptorSyntax("BranchJunction", std::move(options))
	{
	}
};
