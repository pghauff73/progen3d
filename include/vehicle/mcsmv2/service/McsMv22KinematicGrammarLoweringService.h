#pragma once

#include "vehicle/mcsmv2/model/McsMv22KinematicFamilyDefinition.h"
#include "vehicle/mcsmv2/model/ModernCarSemanticFamily.h"

#include <string>

class McsMv22KinematicGrammarLoweringService
{
public:
	std::string lowerVariantClosedPreview(
		const ModernCarSemanticVariant &base_variant,
		const McsMv22KinematicVariantDefinition &kinematic_variant) const;

	std::string lowerVariantOpenPreview(
		const ModernCarSemanticVariant &base_variant,
		const McsMv22KinematicVariantDefinition &kinematic_variant) const;

	std::string lowerFamilyClosedPreview(
		const ModernCarSemanticFamily &base_family,
		const McsMv22KinematicFamilyDefinition &kinematic_family) const;

	std::string lowerFamilyEngineeringPreview(
		const ModernCarSemanticFamily &base_family,
		const McsMv22KinematicFamilyDefinition &kinematic_family) const;

private:
	std::string lowerVariantRule(
		const ModernCarSemanticVariant &base_variant,
		const McsMv22KinematicVariantDefinition &kinematic_variant,
		const std::string &rule_name,
		const std::string &object_identifier,
		bool open_state) const;
};
