#pragma once

enum class PlantDevelopmentState
{
	Seed,
	Bud,
	Shoot,
	Juvenile,
	Mature,
	Flowering,
	Fruiting,
	Senescent,
	Dormant,
	Dead
};

inline const char *plantDevelopmentStateName(PlantDevelopmentState state)
{
	switch (state) {
	case PlantDevelopmentState::Seed: return "Seed";
	case PlantDevelopmentState::Bud: return "Bud";
	case PlantDevelopmentState::Shoot: return "Shoot";
	case PlantDevelopmentState::Juvenile: return "Juvenile";
	case PlantDevelopmentState::Mature: return "Mature";
	case PlantDevelopmentState::Flowering: return "Flowering";
	case PlantDevelopmentState::Fruiting: return "Fruiting";
	case PlantDevelopmentState::Senescent: return "Senescent";
	case PlantDevelopmentState::Dormant: return "Dormant";
	case PlantDevelopmentState::Dead: return "Dead";
	}
	return "Unknown";
}
