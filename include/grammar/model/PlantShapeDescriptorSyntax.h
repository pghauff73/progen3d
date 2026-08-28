#pragma once

#include "grammar/model/ShapeDescriptorSyntax.h"

#include <utility>

class PlantShapeDescriptorSyntax final : public StoredShapeDescriptorSyntax
{
public:
	explicit PlantShapeDescriptorSyntax(std::vector<ShapeOptionSyntax> options)
		: StoredShapeDescriptorSyntax("Plant", std::move(options))
	{
	}
};
