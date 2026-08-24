#pragma once

class PolarDomainSpecification
{
public:
	PolarDomainSpecification(float minimum_degrees = 0.0f,
	                         float maximum_degrees = 180.0f)
		: minimum_degrees_(minimum_degrees),
		  maximum_degrees_(maximum_degrees)
	{
	}

	float minimumDegrees() const
	{
		return minimum_degrees_;
	}

	float maximumDegrees() const
	{
		return maximum_degrees_;
	}

private:
	float minimum_degrees_ = 0.0f;
	float maximum_degrees_ = 180.0f;
};
