#pragma once

#include <cstdint>

class BranchGraph;

class BranchGraphDeterministicHashService
{
public:
	std::uint64_t hash(const BranchGraph &graph) const;
};

