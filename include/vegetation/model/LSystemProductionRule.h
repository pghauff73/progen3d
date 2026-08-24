#pragma once

#include "vegetation/model/LSystemSymbol.h"

#include <utility>
#include <vector>

class LSystemProductionRule
{
public:
	LSystemProductionRule(
		LSystemSymbol predecessor,
		std::vector<LSystemSymbol> successor)
		: predecessor_(predecessor), successor_(std::move(successor))
	{
	}

	LSystemSymbol predecessor() const { return predecessor_; }
	const std::vector<LSystemSymbol> &successor() const { return successor_; }

private:
	LSystemSymbol predecessor_ = LSystemSymbol::Forward;
	std::vector<LSystemSymbol> successor_;
};
