#pragma once

class AxialProfileCapPolicy
{
public:
	static AxialProfileCapPolicy createNone()
	{
		return AxialProfileCapPolicy(false, false);
	}

	static AxialProfileCapPolicy createAll()
	{
		return AxialProfileCapPolicy(true, true);
	}

	static AxialProfileCapPolicy createBottom()
	{
		return AxialProfileCapPolicy(true, false);
	}

	static AxialProfileCapPolicy createTop()
	{
		return AxialProfileCapPolicy(false, true);
	}

	bool capsBottom() const
	{
		return cap_bottom_;
	}

	bool capsTop() const
	{
		return cap_top_;
	}

	bool closesBothEnds() const
	{
		return cap_bottom_ && cap_top_;
	}

private:
	AxialProfileCapPolicy(bool cap_bottom, bool cap_top)
		: cap_bottom_(cap_bottom), cap_top_(cap_top)
	{
	}

	bool cap_bottom_ = false;
	bool cap_top_ = false;
};
