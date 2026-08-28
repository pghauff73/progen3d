#pragma once

class WhorlSpecification
{
public:
	WhorlSpecification(
		int organ_count,
		float radius,
		float phase_degrees,
		float tilt_degrees)
		: organ_count_(organ_count),
		  radius_(radius),
		  phase_degrees_(phase_degrees),
		  tilt_degrees_(tilt_degrees)
	{
	}

	int organCount() const { return organ_count_; }
	float radius() const { return radius_; }
	float phaseDegrees() const { return phase_degrees_; }
	float tiltDegrees() const { return tilt_degrees_; }

private:
	int organ_count_ = 5;
	float radius_ = 0.0f;
	float phase_degrees_ = 0.0f;
	float tilt_degrees_ = 0.0f;
};

