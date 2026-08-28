#pragma once

#include <string>

class ExtrudeProfileCapPolicy
{
public:
	static ExtrudeProfileCapPolicy createAll() { return {true, true}; }
	static ExtrudeProfileCapPolicy createNone() { return {false, false}; }
	static ExtrudeProfileCapPolicy createFront() { return {true, false}; }
	static ExtrudeProfileCapPolicy createBack() { return {false, true}; }

	bool closesFront() const { return closes_front_; }
	bool closesBack() const { return closes_back_; }
	bool closesBothEnds() const { return closes_front_ && closes_back_; }

	std::string canonicalText() const
	{
		if (closes_front_ && closes_back_) return "all";
		if (closes_front_) return "front";
		if (closes_back_) return "back";
		return "none";
	}

private:
	ExtrudeProfileCapPolicy(bool closes_front, bool closes_back)
		: closes_front_(closes_front), closes_back_(closes_back)
	{
	}

	bool closes_front_ = true;
	bool closes_back_ = true;
};
