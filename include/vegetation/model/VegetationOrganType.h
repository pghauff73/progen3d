#pragma once

enum class VegetationOrganType
{
	Branch,
	Bud,
	Leaf,
	Petal,
	Flower,
	Fruit,
	Thorn,
	Petiole,
	Tendril
};

inline const char *vegetationOrganTypeName(VegetationOrganType organ_type)
{
	switch (organ_type) {
	case VegetationOrganType::Branch: return "Branch";
	case VegetationOrganType::Bud: return "Bud";
	case VegetationOrganType::Leaf: return "Leaf";
	case VegetationOrganType::Petal: return "Petal";
	case VegetationOrganType::Flower: return "Flower";
	case VegetationOrganType::Fruit: return "Fruit";
	case VegetationOrganType::Thorn: return "Thorn";
	case VegetationOrganType::Petiole: return "Petiole";
	case VegetationOrganType::Tendril: return "Tendril";
	}
	return "Unknown";
}
