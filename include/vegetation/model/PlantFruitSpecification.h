#pragma once

class PlantFruitSpecification
{
public:
	PlantFruitSpecification() = default;

	PlantFruitSpecification(
		float height,
		float maximum_radius,
		float shoulder_fraction,
		float fullness,
		int radial_segments,
		int profile_segments)
		: enabled_(true),
		  height_(height),
		  maximum_radius_(maximum_radius),
		  shoulder_fraction_(shoulder_fraction),
		  fullness_(fullness),
		  radial_segments_(radial_segments),
		  profile_segments_(profile_segments)
	{
	}

	bool isEnabled() const { return enabled_; }
	float height() const { return height_; }
	float maximumRadius() const { return maximum_radius_; }
	float shoulderFraction() const { return shoulder_fraction_; }
	float fullness() const { return fullness_; }
	int radialSegments() const { return radial_segments_; }
	int profileSegments() const { return profile_segments_; }

private:
	bool enabled_ = false;
	float height_ = 0.10f;
	float maximum_radius_ = 0.055f;
	float shoulder_fraction_ = 0.45f;
	float fullness_ = 4.0f;
	int radial_segments_ = 20;
	int profile_segments_ = 10;
};
