#pragma once

enum class VehicleAxleRole
{
	Front,
	Rear
};

enum class VehicleSideRole
{
	Left,
	Right
};

class VehicleWheelPositionRelationship
{
public:
	VehicleWheelPositionRelationship(VehicleAxleRole axle, VehicleSideRole side)
		: axle_(axle), side_(side)
	{
	}

	VehicleAxleRole axle() const { return axle_; }
	VehicleSideRole side() const { return side_; }

private:
	VehicleAxleRole axle_ = VehicleAxleRole::Front;
	VehicleSideRole side_ = VehicleSideRole::Left;
};
