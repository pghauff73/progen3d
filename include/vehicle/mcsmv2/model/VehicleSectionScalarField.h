#pragma once

#include <cstddef>

enum class VehicleSectionScalarField
{
	UnderbodyHeight,
	UnderbodyHalfWidth,
	RockerHeight,
	RockerHalfWidth,
	LowerBodyHeight,
	LowerBodyHalfWidth,
	ShoulderHeight,
	ShoulderHalfWidth,
	BeltHeight,
	BeltHalfWidth,
	GlassShoulderHeight,
	GlassShoulderHalfWidth,
	RoofRailHeight,
	RoofRailHalfWidth,
	RoofCrownHeight
};

constexpr std::size_t kVehicleSectionScalarFieldCount = 15u;

const char *vehicleSectionScalarFieldName(VehicleSectionScalarField field);
