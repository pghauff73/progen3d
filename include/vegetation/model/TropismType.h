#pragma once

enum class TropismType
{
	Gravity,
	Light,
	Up,
	Support,
	Moisture,
	ObstacleAvoidance,
	Custom
};

inline const char *tropismTypeName(TropismType type)
{
	switch (type) {
	case TropismType::Gravity: return "Gravity";
	case TropismType::Light: return "Light";
	case TropismType::Up: return "Up";
	case TropismType::Support: return "Support";
	case TropismType::Moisture: return "Moisture";
	case TropismType::ObstacleAvoidance: return "ObstacleAvoidance";
	case TropismType::Custom: return "Custom";
	}
	return "Unknown";
}
