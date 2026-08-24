#pragma once

#include "vehicle/mcsmv2/model/ModernCarSemanticFamily.h"

class ModernCarSemanticCatalogFactory
{
public:
	ModernCarSemanticFamily createFamily() const;
};
