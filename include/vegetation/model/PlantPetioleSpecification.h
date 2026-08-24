#pragma once

class PlantPetioleSpecification
{
public:
	PlantPetioleSpecification(
		float length,
		float base_radius,
		float tip_radius)
		: length_(length),
		  base_radius_(base_radius),
		  tip_radius_(tip_radius)
	{
	}

	float length() const { return length_; }
	float baseRadius() const { return base_radius_; }
	float tipRadius() const { return tip_radius_; }

private:
	float length_ = 0.02f;
	float base_radius_ = 0.002f;
	float tip_radius_ = 0.001f;
};
