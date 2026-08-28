#pragma once

class VegetationMeasuredPoint3d
{
public:
	VegetationMeasuredPoint3d(double x, double y, double z)
		: x_(x), y_(y), z_(z)
	{
	}

	double x() const { return x_; }
	double y() const { return y_; }
	double z() const { return z_; }

private:
	double x_ = 0.0;
	double y_ = 0.0;
	double z_ = 0.0;
};
