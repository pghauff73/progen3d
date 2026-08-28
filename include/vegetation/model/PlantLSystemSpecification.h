#pragma once

#include "vegetation/model/LSystemProductionRule.h"

#include <cstddef>
#include <utility>
#include <vector>

class PlantLSystemSpecification
{
public:
	PlantLSystemSpecification(
		std::vector<LSystemSymbol> axiom,
		std::vector<LSystemProductionRule> production_rules,
		std::size_t iteration_count,
		float turn_angle_degrees,
		float step_length,
		float base_radius,
		float terminal_radius,
		float radius_conservation_exponent)
		: axiom_(std::move(axiom)),
		  production_rules_(std::move(production_rules)),
		  iteration_count_(iteration_count),
		  turn_angle_degrees_(turn_angle_degrees),
		  step_length_(step_length),
		  base_radius_(base_radius),
		  terminal_radius_(terminal_radius),
		  radius_conservation_exponent_(radius_conservation_exponent)
	{
	}

	const std::vector<LSystemSymbol> &axiom() const { return axiom_; }
	const std::vector<LSystemProductionRule> &productionRules() const
	{
		return production_rules_;
	}
	std::size_t iterationCount() const { return iteration_count_; }
	float turnAngleDegrees() const { return turn_angle_degrees_; }
	float stepLength() const { return step_length_; }
	float baseRadius() const { return base_radius_; }
	float terminalRadius() const { return terminal_radius_; }
	float radiusConservationExponent() const
	{
		return radius_conservation_exponent_;
	}

private:
	std::vector<LSystemSymbol> axiom_;
	std::vector<LSystemProductionRule> production_rules_;
	std::size_t iteration_count_ = 0u;
	float turn_angle_degrees_ = 25.0f;
	float step_length_ = 0.2f;
	float base_radius_ = 0.05f;
	float terminal_radius_ = 0.006f;
	float radius_conservation_exponent_ = 2.0f;
};
