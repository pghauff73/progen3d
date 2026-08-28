#pragma once

#include <optional>
#include <string>

enum class PlantArchitecture
{
	Tree,
	Shrub,
	Herb,
	Grass,
	Vine
};

inline const char *plantArchitectureName(PlantArchitecture architecture)
{
	switch (architecture) {
	case PlantArchitecture::Tree: return "Tree";
	case PlantArchitecture::Shrub: return "Shrub";
	case PlantArchitecture::Herb: return "Herb";
	case PlantArchitecture::Grass: return "Grass";
	case PlantArchitecture::Vine: return "Vine";
	}
	return "Unknown";
}

inline std::optional<PlantArchitecture> plantArchitectureFromName(
	const std::string &name)
{
	if (name == "Tree") return PlantArchitecture::Tree;
	if (name == "Shrub") return PlantArchitecture::Shrub;
	if (name == "Herb") return PlantArchitecture::Herb;
	if (name == "Grass") return PlantArchitecture::Grass;
	if (name == "Vine") return PlantArchitecture::Vine;
	return std::nullopt;
}
