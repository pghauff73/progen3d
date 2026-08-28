#pragma once

#include "vehicle/mcsmv2/model/McsMv22KinematicFamilyDefinition.h"

class McsMv22KinematicCatalogFactory
{
public:
	McsMv22KinematicFamilyDefinition createFamilyDefinition() const;
};
