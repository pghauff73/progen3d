#pragma once

#include "vegetation/model/PlantGrowthChannelSpecification.h"

class PlantGrowthSpecification
{
public:
	PlantGrowthSpecification(
		float start_time,
		float duration,
		PlantGrowthChannelSpecification length_growth,
		PlantGrowthChannelSpecification radius_growth,
		PlantGrowthChannelSpecification organ_growth)
		: start_time_(start_time),
		  duration_(duration),
		  length_growth_(length_growth),
		  radius_growth_(radius_growth),
		  organ_growth_(organ_growth)
	{
	}

	float startTime() const { return start_time_; }
	float duration() const { return duration_; }
	const PlantGrowthChannelSpecification &lengthGrowth() const
	{
		return length_growth_;
	}
	const PlantGrowthChannelSpecification &radiusGrowth() const
	{
		return radius_growth_;
	}
	const PlantGrowthChannelSpecification &organGrowth() const
	{
		return organ_growth_;
	}

private:
	float start_time_ = 0.0f;
	float duration_ = 1.0f;
	PlantGrowthChannelSpecification length_growth_{0.05f, GrowthCurve::SmoothStep};
	PlantGrowthChannelSpecification radius_growth_{0.08f, GrowthCurve::SmoothStep};
	PlantGrowthChannelSpecification organ_growth_{0.02f, GrowthCurve::EaseOut};
};
