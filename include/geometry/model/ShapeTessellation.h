#pragma once

class ShapeTessellation
{
public:
	ShapeTessellation(int circumferential_segments = 40,
	                  int longitudinal_segments = 1)
		: circumferential_segments_(circumferential_segments),
		  longitudinal_segments_(longitudinal_segments)
	{
	}

	int circumferentialSegments() const
	{
		return circumferential_segments_;
	}

	int longitudinalSegments() const
	{
		return longitudinal_segments_;
	}

private:
	int circumferential_segments_ = 40;
	int longitudinal_segments_ = 1;
};
