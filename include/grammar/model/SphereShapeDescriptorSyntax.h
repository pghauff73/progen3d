#pragma once

#include "grammar/model/ShapeDescriptorSyntax.h"

class SphereShapeDescriptorSyntax : public StoredShapeDescriptorSyntax
{
public:
	explicit SphereShapeDescriptorSyntax(
		std::vector<ShapeOptionSyntax> options = {})
		: StoredShapeDescriptorSyntax("Sphere", std::move(options))
	{
	}
};
