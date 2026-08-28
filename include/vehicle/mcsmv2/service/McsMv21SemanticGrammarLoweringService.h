#pragma once

#include "vehicle/mcsmv2/model/McsMv21SemanticFamilyDefinition.h"
#include "vehicle/mcsmv2/model/ModernCarSemanticFamily.h"

#include <string>

class McsMv21SemanticGrammarLoweringService
{
public:
	std::string lowerFamilyPreview(
		const ModernCarSemanticFamily &base_family,
		const McsMv21SemanticFamilyDefinition &semantic_family) const;
	std::string lowerVariantPreview(
		const ModernCarSemanticVariant &base_variant,
		const McsMv21SemanticVariantDefinition &semantic_variant) const;

private:
	std::string lowerVariantRule(
		const ModernCarSemanticVariant &base_variant,
		const McsMv21SemanticVariantDefinition &semantic_variant,
		const std::string &rule_name,
		const std::string &object_identifier,
		const std::string &container_identifier) const;
};
