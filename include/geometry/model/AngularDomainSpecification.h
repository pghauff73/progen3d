#pragma once

class AngularDomainSpecification
{
public:
	AngularDomainSpecification(float start_degrees = 0.0f,
	                           float sweep_degrees = 360.0f)
		: start_degrees_(start_degrees),
		  sweep_degrees_(sweep_degrees)
	{
	}

	float startDegrees() const
	{
		return start_degrees_;
	}

	float sweepDegrees() const
	{
		return sweep_degrees_;
	}

	bool isFullRevolution() const
	{
		return sweep_degrees_ >= 360.0f;
	}

private:
	float start_degrees_ = 0.0f;
	float sweep_degrees_ = 360.0f;
};
