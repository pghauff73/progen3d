#pragma once

enum class VineGrowthMode
{
	FreeClimbing,
	WallClimbing,
	TrellisClimbing,
	GroundCreeping,
	Hanging,
	Twining,
	TendrilClimbing
};

inline const char *vineGrowthModeName(VineGrowthMode mode)
{
	switch (mode) {
	case VineGrowthMode::FreeClimbing: return "FreeClimbing";
	case VineGrowthMode::WallClimbing: return "WallClimbing";
	case VineGrowthMode::TrellisClimbing: return "TrellisClimbing";
	case VineGrowthMode::GroundCreeping: return "GroundCreeping";
	case VineGrowthMode::Hanging: return "Hanging";
	case VineGrowthMode::Twining: return "Twining";
	case VineGrowthMode::TendrilClimbing: return "TendrilClimbing";
	}
	return "Unknown";
}
