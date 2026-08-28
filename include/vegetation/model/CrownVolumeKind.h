#pragma once

enum class CrownVolumeKind
{
	Sphere,
	Ellipsoid,
	Cone,
	InverseCone,
	Cylinder,
	Dome,
	Lobed,
	CustomSampled
};

inline const char *crownVolumeKindName(CrownVolumeKind kind)
{
	switch (kind) {
	case CrownVolumeKind::Sphere: return "Sphere";
	case CrownVolumeKind::Ellipsoid: return "Ellipsoid";
	case CrownVolumeKind::Cone: return "Cone";
	case CrownVolumeKind::InverseCone: return "InverseCone";
	case CrownVolumeKind::Cylinder: return "Cylinder";
	case CrownVolumeKind::Dome: return "Dome";
	case CrownVolumeKind::Lobed: return "Lobed";
	case CrownVolumeKind::CustomSampled: return "CustomSampled";
	}
	return "Unknown";
}
