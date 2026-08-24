#pragma once

#include <cstdint>

enum class GeometryDetailLevel : std::uint8_t
{
	Bounds = 0,
	CoarseShape = 1,
	Assembly = 2,
	Component = 3,
	ConstructionDetail = 4,
	FastenersAndSeals = 5
};

inline int geometryDetailLevelRank(GeometryDetailLevel detail_level)
{
	return static_cast<int>(detail_level);
}

inline const char *geometryDetailLevelName(GeometryDetailLevel detail_level)
{
	switch (detail_level) {
	case GeometryDetailLevel::Bounds:
		return "Bounds";
	case GeometryDetailLevel::CoarseShape:
		return "CoarseShape";
	case GeometryDetailLevel::Assembly:
		return "Assembly";
	case GeometryDetailLevel::Component:
		return "Component";
	case GeometryDetailLevel::ConstructionDetail:
		return "ConstructionDetail";
	case GeometryDetailLevel::FastenersAndSeals:
		return "FastenersAndSeals";
	}
	return "Unknown";
}
