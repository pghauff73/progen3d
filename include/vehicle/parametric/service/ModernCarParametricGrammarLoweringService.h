#pragma once

#include "vehicle/parametric/model/ModernCarFamilyDefinition.h"
#include "vehicle/parametric/model/ModernCarVariantDefinition.h"

#include <string>

class ModernCarParametricGrammarLoweringService
{
public:
	std::string lowerFamilyPreview(
		const ModernCarFamilyDefinition &family) const;
	std::string lowerVariantPreview(
		const ModernCarVariantDefinition &variant) const;
	std::string lowerMvp26ReferenceAdapter(
		const ModernCarVariantDefinition &reference_variant) const;

private:
	std::string lowerVehicleCandidateRules() const;
	std::string lowerVariantRule(
		const ModernCarVariantDefinition &variant,
		const std::string &rule_name,
		double placement_z,
		const std::string &container_identifier) const;
};
