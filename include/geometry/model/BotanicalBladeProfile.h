#pragma once

enum class BotanicalBladeProfile
{
	Linear,
	Lanceolate,
	Elliptic,
	Ovate,
	Obovate,
	Cordate,
	Palmate,
	Needle
};

inline const char *botanicalBladeProfileName(BotanicalBladeProfile profile)
{
	switch (profile) {
	case BotanicalBladeProfile::Linear:
		return "Linear";
	case BotanicalBladeProfile::Lanceolate:
		return "Lanceolate";
	case BotanicalBladeProfile::Elliptic:
		return "Elliptic";
	case BotanicalBladeProfile::Ovate:
		return "Ovate";
	case BotanicalBladeProfile::Obovate:
		return "Obovate";
	case BotanicalBladeProfile::Cordate:
		return "Cordate";
	case BotanicalBladeProfile::Palmate:
		return "Palmate";
	case BotanicalBladeProfile::Needle:
		return "Needle";
	}
	return "Unknown";
}

