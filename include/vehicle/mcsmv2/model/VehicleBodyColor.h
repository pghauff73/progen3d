#pragma once

class VehicleBodyColor
{
public:
	VehicleBodyColor(int red, int green, int blue)
		: red_(red), green_(green), blue_(blue)
	{
	}

	int red() const { return red_; }
	int green() const { return green_; }
	int blue() const { return blue_; }

private:
	int red_ = 0;
	int green_ = 0;
	int blue_ = 0;
};
