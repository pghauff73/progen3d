#pragma once

#include "chair/model/ChairObjectModel.h"

class ChairAssemblyBuilder
{
public:
	ChairObjectModelBuildResult build(const ChairDesignDefinition &definition) const;
	ChairObjectModelBuildResult build(
		const ChairDesignDefinition &definition,
		const ChairPlacement &placement) const;
};
