#pragma once

#include "vegetation/model/GrowthCurve.h"

class PlantGrowthChannelSpecification
{
public:
	PlantGrowthChannelSpecification(
		float initial_factor,
		GrowthCurve curve)
		: initial_factor_(initial_factor), curve_(curve)
	{
	}

	float initialFactor() const { return initial_factor_; }
	GrowthCurve curve() const { return curve_; }

private:
	float initial_factor_ = 0.0f;
	GrowthCurve curve_ = GrowthCurve::Linear;
};
