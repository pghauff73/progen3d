#include "TimelineSettings.h"

#include "physics/model/SimulationStep.h"
#include <json/json.h>

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

std::uint32_t read_count(const Json::Value &value, const char *field,
                         std::uint32_t minimum, std::uint32_t maximum)
{
	if ((value.type() != Json::intValue && value.type() != Json::uintValue) ||
	    !value.isUInt64() || value.asUInt64() < minimum || value.asUInt64() > maximum) {
		throw std::runtime_error(std::string(field) + " must be an integer from " +
		                         std::to_string(minimum) + " to " + std::to_string(maximum) + ".");
	}
	return static_cast<std::uint32_t>(value.asUInt64());
}

}

TimelineSettings TimelineSettings::parse(const Json::Value &value)
{
	if (!value.isObject()) {
		throw std::runtime_error("timeline must be a JSON object.");
	}
	for (const std::string &field : value.getMemberNames()) {
		if (field != "fps" && field != "frame_count" && field != "start_frame" &&
		    field != "end_frame" && field != "physics" && field != "shutter" && field != "export_transforms") {
			throw std::runtime_error("Unknown field: timeline." + field);
		}
	}

	TimelineSettings timeline;
	timeline.frames_per_second = read_count(value["fps"], "timeline.fps", 1, kSceneSimulationTicksPerSecond);
	timeline.frame_count = read_count(value["frame_count"], "timeline.frame_count", 1,
	                                  std::numeric_limits<std::uint32_t>::max());
	if (value.isMember("start_frame")) {
		timeline.start_frame = read_count(value["start_frame"], "timeline.start_frame", 0, timeline.frame_count - 1);
	}
	timeline.end_frame = timeline.frame_count;
	if (value.isMember("end_frame")) {
		timeline.end_frame = read_count(value["end_frame"], "timeline.end_frame", timeline.start_frame + 1,
		                                timeline.frame_count);
	}
	if (value.isMember("physics")) {
		if (!value["physics"].isBool()) {
			throw std::runtime_error("timeline.physics must be a boolean.");
		}
		timeline.physics = value["physics"].asBool();
	}
	if (value.isMember("export_transforms")) {
		if (!value["export_transforms"].isBool()) {
			throw std::runtime_error("timeline.export_transforms must be a boolean.");
		}
		timeline.export_transforms = value["export_transforms"].asBool();
	}
	if (value.isMember("shutter")) {
		const Json::Value &config = value["shutter"];
		if (!config.isObject()) {
			throw std::runtime_error("timeline.shutter must be a JSON object.");
		}
		for (const std::string &field : config.getMemberNames()) {
			if (field != "frame_fraction" && field != "samples") {
				throw std::runtime_error("Unknown field: timeline.shutter." + field);
			}
		}
		const Json::Value &fraction = config["frame_fraction"];
		if (!fraction.isNumeric() || !std::isfinite(fraction.asDouble()) ||
		    fraction.asDouble() < 0.0 || fraction.asDouble() > 1.0) {
			throw std::runtime_error("timeline.shutter.frame_fraction must be a number from 0 to 1.");
		}
		timeline.shutter = ShutterSettings{
			fraction.asDouble(), read_count(config["samples"], "timeline.shutter.samples", 2, 64)};
	}
	return timeline;
}

double ShutterSettings::sampleTime(double frame_time, double frame_duration, std::uint32_t index) const
{
	return frame_time + frame_duration * frame_fraction * (static_cast<double>(index) + 0.5) / samples;
}

double TimelineSettings::frameTime(std::uint32_t frame) const
{
	return static_cast<double>(frame) / frames_per_second;
}
