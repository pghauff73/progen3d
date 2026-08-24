#pragma once

#include "vehicle/parametric/model/ModernCarVariantDefinition.h"

#include <string>
#include <utility>
#include <vector>

class ModernCarFamilyDefinition
{
public:
	ModernCarFamilyDefinition(
		std::string identifier,
		std::vector<ModernCarVariantDefinition> variants)
		: identifier_(std::move(identifier)), variants_(std::move(variants))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const std::vector<ModernCarVariantDefinition> &variants() const { return variants_; }

	const ModernCarVariantDefinition *findVariant(const std::string &identifier) const
	{
		for (const ModernCarVariantDefinition &variant : variants_) {
			if (variant.identifier() == identifier) return &variant;
		}
		return nullptr;
	}

private:
	std::string identifier_;
	std::vector<ModernCarVariantDefinition> variants_;
};
