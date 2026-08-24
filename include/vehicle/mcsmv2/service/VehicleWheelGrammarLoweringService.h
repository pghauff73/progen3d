#pragma once

#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"

#include <string>

class VehicleWheelGrammarLoweringService
{
public:
	std::string lowerWheelRule(
		const ModernCarSemanticVariant &variant,
		const std::string &rule_name) const;
};
