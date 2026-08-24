#pragma once

#include <cstdint>
#include <string>

class SceneGenerationRequest
{
public:
	std::uint64_t request_id = 0;
	std::string source_text;
	std::uint64_t design_nonce = 0;
	double evaluation_time = 0.0;
	bool time_sample = false;
};
