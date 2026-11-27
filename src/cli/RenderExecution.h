#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <stdexcept>

inline constexpr int render_cancelled_exit_code = 6;

class RenderCancelled : public std::runtime_error {
public:
	RenderCancelled() : std::runtime_error("Render cancelled.") {}
};

struct RenderProgress {
	std::uint32_t rendered_frames = 0;
	std::uint32_t reused_frames = 0;

	std::uint32_t completedFrames() const { return rendered_frames + reused_frames; }
};

struct RenderExecution {
	std::filesystem::path checkpoint_directory;
	std::filesystem::path grammar_snapshot;
	bool resume = false;
	std::function<bool()> cancelled;
	std::function<void(const RenderProgress &)> publish_progress;

	void checkCancellation() const
	{
		if (cancelled && cancelled()) throw RenderCancelled();
	}
};
