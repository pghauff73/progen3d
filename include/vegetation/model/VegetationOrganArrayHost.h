#pragma once

enum class VegetationOrganArrayHost
{
	Stem,
	Branch,
	FlowerHead,
	Surface
};

inline const char *vegetationOrganArrayHostName(VegetationOrganArrayHost host)
{
	switch (host) {
	case VegetationOrganArrayHost::Stem: return "Stem";
	case VegetationOrganArrayHost::Branch: return "Branch";
	case VegetationOrganArrayHost::FlowerHead: return "FlowerHead";
	case VegetationOrganArrayHost::Surface: return "Surface";
	}
	return "Unknown";
}
