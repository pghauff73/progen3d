#pragma once

#include "grammar/model/ShapeDescriptorSyntax.h"

#include <utility>

class VineShapeDescriptorSyntax final : public StoredShapeDescriptorSyntax
{
public:
	explicit VineShapeDescriptorSyntax(std::vector<ShapeOptionSyntax> options)
		: StoredShapeDescriptorSyntax("Vine", std::move(options))
	{
	}
};
