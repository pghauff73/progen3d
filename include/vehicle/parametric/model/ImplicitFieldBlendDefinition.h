#pragma once

class ImplicitFieldBlendDefinition
{
public:
	ImplicitFieldBlendDefinition(
		double lower_greenhouse_smoothness,
		double fender_smoothness)
		: lower_greenhouse_smoothness_(lower_greenhouse_smoothness),
		  fender_smoothness_(fender_smoothness)
	{
	}

	double lowerGreenhouseSmoothness() const
	{
		return lower_greenhouse_smoothness_;
	}
	double fenderSmoothness() const { return fender_smoothness_; }

private:
	double lower_greenhouse_smoothness_ = 0.0;
	double fender_smoothness_ = 0.0;
};
