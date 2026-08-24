#pragma once

#include "geometry/model/ImplicitScalarField.h"
#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"

#include <memory>

class ModernCarSemanticBodyFieldFactory
{
public:
	std::shared_ptr<const ImplicitScalarField> createBodyField(
		const ModernCarSemanticVariant &variant) const;
};
