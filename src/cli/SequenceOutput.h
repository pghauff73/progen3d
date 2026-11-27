#pragma once

#include "FrameOutput.h"
#include "RenderFiles.h"
#include "RenderSceneTimeline.h"
#include "TimelineSettings.h"

#include <json/json.h>

#include <filesystem>
#include <fstream>

class SequenceOutput
{
public:
	SequenceOutput(const std::filesystem::path &root, const TimelineSettings &timeline,
	               const Json::Value &job_report, const std::filesystem::path &checkpoint = {});

	void recordContacts(std::uint64_t tick, const std::vector<TimelineContact> &contacts);
	void publishFrame(std::uint32_t frame_number, const RenderedFrame &frame, Json::Value metadata);
	void reuseFrame(std::uint32_t frame_number, bool overlay = false);
	std::filesystem::path finish(std::uint64_t solved_tick, Json::Value audio_cues);

	const std::filesystem::path &directory() const { return directory_; }

private:
	void indexFrame(std::uint32_t frame_number, const std::filesystem::path &manifest, bool overlay);

	std::filesystem::path directory_;
	render_files::AtomicFile pending_contacts_;
	std::ofstream contact_stream_;
	Json::Value manifest_;
	std::size_t contact_count_ = 0;
};

Json::Value timeline_report(const TimelineSettings &timeline);
Json::Value sample_transforms(const TimelineRenderSample &sample);
