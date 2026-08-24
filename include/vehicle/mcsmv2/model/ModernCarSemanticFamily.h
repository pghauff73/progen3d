#pragma once

#include "vehicle/mcsmv2/model/McsMv201SourceRelease.h"
#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"

#include <string>
#include <utility>
#include <vector>

class ModernCarSemanticFamily
{
public:
	ModernCarSemanticFamily(
		std::string identifier,
		McsMv201SourceRelease source_release,
		std::vector<ModernCarSemanticVariant> variants)
		: identifier_(std::move(identifier)),
		  source_release_(std::move(source_release)),
		  variants_(std::move(variants))
	{
	}

	const std::string &identifier() const { return identifier_; }
	const McsMv201SourceRelease &sourceRelease() const { return source_release_; }
	const std::vector<ModernCarSemanticVariant> &variants() const { return variants_; }

private:
	std::string identifier_;
	McsMv201SourceRelease source_release_{"", "", "", "", 0u};
	std::vector<ModernCarSemanticVariant> variants_;
};
