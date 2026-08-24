#pragma once

#include "grammar/model/ShapeDescriptorSyntax.h"

class CylinderShapeDescriptorSyntax : public StoredShapeDescriptorSyntax
{
public:
	explicit CylinderShapeDescriptorSyntax(
		std::vector<ShapeOptionSyntax> options = {})
		: StoredShapeDescriptorSyntax("Cylinder", std::move(options))
	{
	}
};
