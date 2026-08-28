#pragma once

#include "editor/model/PreviewTimelineState.h"

class PreviewTimelineAdvance
{
public:
	bool stop_playback = false;
	bool request_grammar_sample = false;
	double grammar_sample_time = 0.0;
	float physics_delta_seconds = 0.0f;
};

class PreviewTimelineController
{
public:
	PreviewTimelineAdvance advance(PreviewTimelineState &timeline,
	                               float frame_delta_seconds,
	                               bool grammar_time_driven,
	                               bool generation_in_progress) const;
	double stepGrammarTime(PreviewTimelineState &timeline) const;
	float stepPhysicsTime(const PreviewTimelineState &timeline) const;
	void reset(PreviewTimelineState &timeline) const;
	void pause(PreviewTimelineState &timeline) const;
};
