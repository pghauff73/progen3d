#pragma once

class WheelhouseDifferenceDefinition
{
public:
	explicit WheelhouseDifferenceDefinition(double clearance)
		: clearance_(clearance)
	{
	}

	double clearance() const { return clearance_; }

private:
	double clearance_ = 0.0;
};
