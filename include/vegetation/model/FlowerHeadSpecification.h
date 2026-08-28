#pragma once

class FlowerHeadSpecification
{
public:
	FlowerHeadSpecification() = default;

	FlowerHeadSpecification(
		int floret_count,
		float radius,
		float divergence_degrees,
		float phase_degrees,
		float tilt_degrees,
		float scale_falloff)
		: enabled_(true),
		  floret_count_(floret_count),
		  radius_(radius),
		  divergence_degrees_(divergence_degrees),
		  phase_degrees_(phase_degrees),
		  tilt_degrees_(tilt_degrees),
		  scale_falloff_(scale_falloff)
	{
	}

	bool isEnabled() const { return enabled_; }
	int floretCount() const { return floret_count_; }
	float radius() const { return radius_; }
	float divergenceDegrees() const { return divergence_degrees_; }
	float phaseDegrees() const { return phase_degrees_; }
	float tiltDegrees() const { return tilt_degrees_; }
	float scaleFalloff() const { return scale_falloff_; }

private:
	bool enabled_ = false;
	int floret_count_ = 0;
	float radius_ = 0.0f;
	float divergence_degrees_ = 137.50776f;
	float phase_degrees_ = 0.0f;
	float tilt_degrees_ = 0.0f;
	float scale_falloff_ = 1.0f;
};
