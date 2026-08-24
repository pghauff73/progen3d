#pragma once

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
