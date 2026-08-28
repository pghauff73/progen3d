#pragma once

class AxialDomainSpecification
{
public:
	AxialDomainSpecification(float minimum_position = 0.0f,
	                         float maximum_position = 1.0f)
		: minimum_position_(minimum_position),
		  maximum_position_(maximum_position)
	{
	}

	float minimumPosition() const
	{
		return minimum_position_;
	}

	float maximumPosition() const
	{
		return maximum_position_;
	}

private:
	float minimum_position_ = 0.0f;
	float maximum_position_ = 1.0f;
};
