#pragma once

#include "vehicle/mcsmv2/model/McsMv220SourceRelease.h"
#include "vehicle/mcsmv2/model/McsMv22KinematicVariantDefinition.h"

#include <utility>
#include <vector>

class McsMv22KinematicFamilyDefinition
{
public:
	McsMv22KinematicFamilyDefinition(
		McsMv220SourceRelease source_release,
		std::vector<McsMv22KinematicVariantDefinition> variants)
		: source_release_(std::move(source_release)),
		  variants_(std::move(variants))
	{
	}

	const McsMv220SourceRelease &sourceRelease() const { return source_release_; }
	const std::vector<McsMv22KinematicVariantDefinition> &variants() const
	{
		return variants_;
	}

private:
	McsMv220SourceRelease source_release_{"", "", "", "", 0u, 0u, "", glm::dmat4(1.0), ""};
	std::vector<McsMv22KinematicVariantDefinition> variants_;
};
