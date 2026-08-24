#pragma once

#include <cstddef>

enum class VehicleSectionLandmarkRole
{
	Underbody,
	Rocker,
	LowerBody,
	Shoulder,
	Belt,
	GlassShoulder,
	RoofRail,
	RoofCrown
};

constexpr std::size_t kVehicleSectionLandmarkCount = 8u;

const char *vehicleSectionLandmarkRoleName(VehicleSectionLandmarkRole role);
