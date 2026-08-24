#pragma once

enum class PlantTopologyGenerationMethod
{
	RuleBranching,
	SpaceColonization,
	LSystem
};

inline const char *plantTopologyGenerationMethodName(
	PlantTopologyGenerationMethod method)
{
	switch (method) {
	case PlantTopologyGenerationMethod::RuleBranching:
		return "RuleBranching";
	case PlantTopologyGenerationMethod::SpaceColonization:
		return "SpaceColonization";
	case PlantTopologyGenerationMethod::LSystem:
		return "LSystem";
	}
	return "Unknown";
}
