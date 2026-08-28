#pragma once

#include "grammar/model/ShapeDescriptorSyntax.h"

#include <utility>

class ScatterRegionShapeDescriptorSyntax final : public StoredShapeDescriptorSyntax
{
public:
	explicit ScatterRegionShapeDescriptorSyntax(
		std::vector<ShapeOptionSyntax> options)
		: StoredShapeDescriptorSyntax("ScatterRegion", std::move(options))
	{
	}
};
