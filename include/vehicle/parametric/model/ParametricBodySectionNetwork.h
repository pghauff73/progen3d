#pragma once

#include <cstddef>

class ParametricBodySectionNetwork
{
public:
	explicit ParametricBodySectionNetwork(std::size_t section_count)
		: section_count_(section_count)
	{
	}

	std::size_t sectionCount() const { return section_count_; }

private:
	std::size_t section_count_ = 0u;
};
