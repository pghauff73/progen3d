#pragma once

#include "vehicle/parametric/model/ModernCarParametricObjectModel.h"

class ModernCarParametricCatalogFactory
{
public:
	ModernCarFamilyDefinition createFamilyDefinition() const;
	ParametricModelSourceManifest createSourceManifest() const;
	ModernCarParametricObjectModel createObjectModel(
		ParametricModelGenerationPolicy generation_policy) const;
};
