#pragma once

#include "grammar/model/ShapeDescriptorSyntax.h"

#include <utility>

class TaperedSweepShapeDescriptorSyntax final : public StoredShapeDescriptorSyntax
{
public:
	explicit TaperedSweepShapeDescriptorSyntax(
		std::vector<ShapeOptionSyntax> options)
		: StoredShapeDescriptorSyntax("TaperedSweep", std::move(options))
	{
	}
};
