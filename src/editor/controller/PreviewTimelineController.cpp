#include "editor/controller/PreviewTimelineController.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr double kGrammarSampleStep = 1.0 / 60.0;

}

PreviewTimelineAdvance PreviewTimelineController::advance(
	PreviewTimelineState &timeline,
	float frame_delta_seconds,
	bool grammar_time_driven,
	bool generation_in_progress) const
{
	PreviewTimelineAdvance advance;
	if (!timeline.playing) {
		return advance;
	}
	const double scaled_delta =
		static_cast<double>(frame_delta_seconds) * static_cast<double>(timeline.time_scale);
	if (!grammar_time_driven) {
		if (!generation_in_progress) {
			advance.physics_delta_seconds = static_cast<float>(scaled_delta);
		}
		return advance;
	}

	const double next_time = timeline.grammar_target_time + scaled_delta;
	if (!std::isfinite(next_time) ||
	    std::fabs(next_time) > static_cast<double>(std::numeric_limits<float>::max())) {
		timeline.playing = false;
		advance.stop_playback = true;
		return advance;
	}
	timeline.grammar_target_time = next_time;
	timeline.grammar_request_accumulator +=
		std::max(0.0, static_cast<double>(frame_delta_seconds));
	if (!generation_in_progress &&
	    timeline.grammar_request_accumulator >= kGrammarSampleStep) {
		timeline.grammar_request_accumulator =
			std::fmod(timeline.grammar_request_accumulator, kGrammarSampleStep);
		advance.request_grammar_sample = true;
		advance.grammar_sample_time = timeline.grammar_target_time;
	}
	return advance;
}

double PreviewTimelineController::stepGrammarTime(PreviewTimelineState &timeline) const
{
	timeline.playing = false;
	timeline.grammar_target_time =
		std::max(timeline.grammar_target_time, 0.0) +
		kGrammarSampleStep * static_cast<double>(timeline.time_scale);
	timeline.grammar_request_accumulator = 0.0;
	return timeline.grammar_target_time;
}

float PreviewTimelineController::stepPhysicsTime(const PreviewTimelineState &timeline) const
{
	return static_cast<float>(kGrammarSampleStep) * timeline.time_scale;
}

void PreviewTimelineController::reset(PreviewTimelineState &timeline) const
{
	timeline.playing = false;
	timeline.simulation_accumulator = 0.0f;
	timeline.simulation_alpha = 1.0f;
	timeline.grammar_target_time = 0.0;
	timeline.grammar_request_accumulator = 0.0;
}

void PreviewTimelineController::pause(PreviewTimelineState &timeline) const
{
	timeline.playing = false;
}
