#pragma once

#include "vegetation/model/InflorescenceKind.h"

class InflorescenceSpecification
{
public:
	InflorescenceSpecification() = default;

	InflorescenceSpecification(
		InflorescenceKind kind,
		int flower_count,
		float spacing,
		float radial_extent,
		float phase_degrees,
		float tilt_degrees,
		float scale_falloff)
		: enabled_(true),
		  kind_(kind),
		  flower_count_(flower_count),
		  spacing_(spacing),
		  radial_extent_(radial_extent),
		  phase_degrees_(phase_degrees),
		  tilt_degrees_(tilt_degrees),
		  scale_falloff_(scale_falloff)
	{
	}

	bool isEnabled() const { return enabled_; }
	InflorescenceKind kind() const { return kind_; }
	int flowerCount() const { return flower_count_; }
	float spacing() const { return spacing_; }
	float radialExtent() const { return radial_extent_; }
	float phaseDegrees() const { return phase_degrees_; }
	float tiltDegrees() const { return tilt_degrees_; }
	float scaleFalloff() const { return scale_falloff_; }

private:
	bool enabled_ = false;
	InflorescenceKind kind_ = InflorescenceKind::Raceme;
	int flower_count_ = 0;
	float spacing_ = 0.0f;
	float radial_extent_ = 0.0f;
	float phase_degrees_ = 0.0f;
	float tilt_degrees_ = 0.0f;
	float scale_falloff_ = 1.0f;
};
