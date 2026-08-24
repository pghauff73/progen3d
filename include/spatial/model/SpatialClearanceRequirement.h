#pragma once

class SpatialClearanceRequirement {
public:
	SpatialClearanceRequirement(float nominal, float minimum, float maximum)
		: nominal_(nominal), minimum_(minimum), maximum_(maximum) {}

	float nominal() const { return nominal_; }
	float minimum() const { return minimum_; }
	float maximum() const { return maximum_; }

	bool accepts(float measured_clearance, float tolerance) const
	{
		return measured_clearance >= minimum_ - tolerance &&
		       measured_clearance <= maximum_ + tolerance;
	}

private:
	float nominal_ = 0.0f;
	float minimum_ = 0.0f;
	float maximum_ = 0.0f;
};
