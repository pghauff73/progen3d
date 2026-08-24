#pragma once

#include "vehicle/mcsmv2/model/McsMv210SourceRelease.h"
#include "vehicle/mcsmv2/model/McsMv21SemanticVariantDefinition.h"

#include <utility>
#include <vector>

class McsMv21SemanticFamilyDefinition
{
public:
	McsMv21SemanticFamilyDefinition(
		McsMv210SourceRelease source_release,
		std::vector<McsMv21SemanticVariantDefinition> variants)
		: source_release_(std::move(source_release)),
		  variants_(std::move(variants))
	{
	}

	const McsMv210SourceRelease &sourceRelease() const { return source_release_; }
	const std::vector<McsMv21SemanticVariantDefinition> &variants() const
	{
		return variants_;
	}

private:
	McsMv210SourceRelease source_release_{"", "", "", "", 0u};
	std::vector<McsMv21SemanticVariantDefinition> variants_;
};
