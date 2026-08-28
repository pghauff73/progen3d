#pragma once

enum class InflorescenceKind
{
	Raceme,
	Spike,
	Panicle,
	Umbel,
	Corymb,
	Head
};

inline const char *inflorescenceKindName(InflorescenceKind kind)
{
	switch (kind) {
	case InflorescenceKind::Raceme: return "Raceme";
	case InflorescenceKind::Spike: return "Spike";
	case InflorescenceKind::Panicle: return "Panicle";
	case InflorescenceKind::Umbel: return "Umbel";
	case InflorescenceKind::Corymb: return "Corymb";
	case InflorescenceKind::Head: return "Head";
	}
	return "Unknown";
}
