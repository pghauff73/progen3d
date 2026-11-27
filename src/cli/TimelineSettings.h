#pragma once

#include <cstdint>
#include <optional>

namespace Json {
class Value;
}

struct ShutterSettings {
	double frame_fraction = 0.0;
	std::uint32_t samples = 0;

	double sampleTime(double frame_time, double frame_duration, std::uint32_t index) const;
};

class TimelineSettings
{
public:
	static TimelineSettings parse(const Json::Value &value);

	double frameTime(std::uint32_t frame) const;

	std::uint32_t frames_per_second = 24;
	std::uint32_t frame_count = 0;
	std::uint32_t start_frame = 0;
	std::uint32_t end_frame = 0;
	bool physics = false;
	bool export_transforms = true;
	std::optional<ShutterSettings> shutter;
};
