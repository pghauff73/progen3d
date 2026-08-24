#pragma once

enum class VehicleSemanticStationRole
{
	TailFace,
	RearBumper,
	RearAxle,
	RearDoor,
	BPillar,
	FrontDoor,
	APillar,
	HoodRear,
	FrontAxle,
	Nose,
	FrontFace
};

const char *vehicleSemanticStationRoleName(VehicleSemanticStationRole role);
