#pragma once

class RadialDomainSpecification
{
public:
	RadialDomainSpecification(float minimum_radius_fraction = 0.0f,
	                          float maximum_radius_fraction = 1.0f)
		: minimum_radius_fraction_(minimum_radius_fraction),
		  maximum_radius_fraction_(maximum_radius_fraction)
	{
	}

	float minimumRadiusFraction() const
	{
		return minimum_radius_fraction_;
	}

	float maximumRadiusFraction() const
	{
		return maximum_radius_fraction_;
	}

private:
	float minimum_radius_fraction_ = 0.0f;
	float maximum_radius_fraction_ = 1.0f;
};
