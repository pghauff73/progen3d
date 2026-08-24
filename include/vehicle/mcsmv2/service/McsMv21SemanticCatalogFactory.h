#pragma once

#include "vehicle/mcsmv2/model/McsMv21SemanticFamilyDefinition.h"

class McsMv21SemanticCatalogFactory
{
public:
	McsMv21SemanticFamilyDefinition createFamilyDefinition() const;
};
