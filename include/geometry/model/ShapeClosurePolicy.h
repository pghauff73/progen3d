#pragma once

#include <string>

class ShapeClosurePolicy
{
public:
	static ShapeClosurePolicy createNone()
	{
		return ShapeClosurePolicy();
	}

	static ShapeClosurePolicy createAll()
	{
		ShapeClosurePolicy policy;
		policy.axial_ = true;
		policy.angular_ = true;
		policy.clip_ = true;
		policy.rim_ = true;
		return policy;
	}

	void includeAxial()
	{
		axial_ = true;
	}

	void includeAngular()
	{
		angular_ = true;
	}

	void includeClip()
	{
		clip_ = true;
	}

	void includeRim()
	{
		rim_ = true;
	}

	bool closesAxialBoundaries() const
	{
		return axial_;
	}

	bool closesAngularBoundaries() const
	{
		return angular_;
	}

	bool closesClipBoundaries() const
	{
		return clip_;
	}

	bool closesRims() const
	{
		return rim_;
	}

	bool isEmpty() const
	{
		return !axial_ && !angular_ && !clip_ && !rim_;
	}

	std::string canonicalText() const;

private:
	bool axial_ = false;
	bool angular_ = false;
	bool clip_ = false;
	bool rim_ = false;
};
