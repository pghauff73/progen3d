#pragma once

#include "vegetation/model/PlantLSystemSpecification.h"
#include "vegetation/model/PlantSpaceColonizationSpecification.h"
#include "vegetation/model/PlantTopologyGenerationMethod.h"

#include <optional>
#include <utility>

class PlantTopologyGenerationSpecification
{
public:
	static PlantTopologyGenerationSpecification createRuleBranching()
	{
		return PlantTopologyGenerationSpecification(
			PlantTopologyGenerationMethod::RuleBranching, std::nullopt,
			std::nullopt);
	}

	static PlantTopologyGenerationSpecification createSpaceColonization(
		PlantSpaceColonizationSpecification specification)
	{
		return PlantTopologyGenerationSpecification(
			PlantTopologyGenerationMethod::SpaceColonization,
			std::move(specification), std::nullopt);
	}

	static PlantTopologyGenerationSpecification createLSystem(
		PlantLSystemSpecification specification)
	{
		return PlantTopologyGenerationSpecification(
			PlantTopologyGenerationMethod::LSystem, std::nullopt,
			std::move(specification));
	}

	PlantTopologyGenerationMethod method() const { return method_; }
	const std::optional<PlantSpaceColonizationSpecification> &spaceColonization()
		const
	{
		return space_colonization_;
	}
	const std::optional<PlantLSystemSpecification> &lSystem() const
	{
		return l_system_;
	}

private:
	PlantTopologyGenerationSpecification(
		PlantTopologyGenerationMethod method,
		std::optional<PlantSpaceColonizationSpecification> space_colonization,
		std::optional<PlantLSystemSpecification> l_system)
		: method_(method),
		  space_colonization_(std::move(space_colonization)),
		  l_system_(std::move(l_system))
	{
	}

	PlantTopologyGenerationMethod method_ =
		PlantTopologyGenerationMethod::RuleBranching;
	std::optional<PlantSpaceColonizationSpecification> space_colonization_;
	std::optional<PlantLSystemSpecification> l_system_;
};
