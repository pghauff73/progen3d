#pragma once

#include "vehicle/mcsmv2/model/ModernCarSemanticFamily.h"
#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"

#include <string>

class ModernCarSemanticGrammarLoweringService
{
public:
	std::string lowerFamilyPreview(
		const ModernCarSemanticFamily &family) const;
	std::string lowerVariantPreview(
		const ModernCarSemanticVariant &variant) const;

private:
	std::string lowerVariantRule(
		const ModernCarSemanticVariant &variant,
		const std::string &rule_name,
		const std::string &object_identifier,
		const std::string &container_identifier) const;
};
