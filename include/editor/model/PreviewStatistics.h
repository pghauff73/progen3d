#pragma once

#include <cstddef>

class PreviewStatistics
{
public:
	std::size_t rule_count = 0;
	std::size_t token_count = 0;
	std::size_t primitive_count = 0;
	std::size_t material_count = 0;
	double generation_milliseconds = 0.0;
	double render_milliseconds = 0.0;
	int benchmark_redraws = 0;
	double benchmark_viewport_milliseconds = 0.0;
};
