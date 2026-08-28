#pragma once

class PreviewTimelineState
{
public:
	bool playing = false;
	float time_scale = 1.0f;
	float simulation_accumulator = 0.0f;
	float simulation_alpha = 1.0f;
	double grammar_target_time = 0.0;
	double grammar_request_accumulator = 0.0;
};
