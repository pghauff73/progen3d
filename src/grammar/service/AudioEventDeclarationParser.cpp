#include "grammar/service/AudioEventDeclarationParser.h"

#include <charconv>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace {

constexpr std::size_t max_speech_bytes = 1024;

bool isSpace(char value)
{
	return std::isspace(static_cast<unsigned char>(value)) != 0;
}

bool isIdentifierCharacter(char value)
{
	return std::isalnum(static_cast<unsigned char>(value)) != 0 ||
	       value == '_' || value == '-' || value == '.';
}

class AudioEventSourceReader {
public:
	explicit AudioEventSourceReader(std::string_view source) : source_(source) {}

	std::size_t column() const { return column_; }

	void expect(char expected)
	{
		if (!take(expected)) fail(std::string("Expected '") + expected + "'.");
	}

	bool take(char expected)
	{
		skipSpace();
		if (column_ == source_.size() || source_[column_] != expected) return false;
		++column_;
		return true;
	}

	std::string identifier(const char *description)
	{
		skipSpace();
		const std::size_t start = column_;
		while (column_ < source_.size() && isIdentifierCharacter(source_[column_])) ++column_;
		if (column_ == start) fail(std::string("Expected ") + description + ".");
		return std::string(source_.substr(start, column_ - start));
	}

	template <typename Number>
	Number number(const char *description)
	{
		skipSpace();
		const char *start = source_.data() + column_;
		const char *end = source_.data() + source_.size();
		Number value{};
		const auto result = std::from_chars(start, end, value);
		if (result.ec != std::errc() || !std::isfinite(value) ||
		    (result.ptr != end && !isSpace(*result.ptr) && *result.ptr != ')')) {
			fail(std::string("Expected a finite ") + description + ".");
		}
		column_ += static_cast<std::size_t>(result.ptr - start);
		return value;
	}

	std::string speech()
	{
		expect('"');
		std::string text;
		while (column_ < source_.size()) {
			char value = source_[column_++];
			if (value == '"') {
				if (text.find_first_not_of(" \t") == std::string::npos)
					fail("Speech text must contain words.");
				return text;
			}
			if (value == '\\' && column_ < source_.size()) {
				value = source_[column_++];
				if (value != '"' && value != '\\')
					fail("Only escaped quotes and backslashes are supported in speech text.");
			}
			if (static_cast<unsigned char>(value) < 32 && value != '\t')
				fail("Unexpected control character in speech text.");
			text += value;
			if (text.size() > max_speech_bytes) fail("Speech text exceeds 1024 UTF-8 bytes.");
		}
		fail("Unclosed speech text.");
	}

	bool atEnd()
	{
		skipSpace();
		const auto remainder = source_.substr(column_);
		return remainder.empty() || remainder.front() == '#' || remainder.substr(0, 2) == "//";
	}

	[[noreturn]] void fail(const std::string &message) const
	{
		throw std::invalid_argument(message);
	}

private:
	void skipSpace()
	{
		while (column_ < source_.size() && isSpace(source_[column_])) ++column_;
	}

	std::string_view source_;
	std::size_t column_ = 0;
};

SimulationAudioTrigger readSimulationTrigger(AudioEventSourceReader &reader)
{
	const std::string event = reader.identifier("a simulation event");
	if (event == "start") return {SimulationAudioEvent::Start, {}};
	SimulationAudioEvent kind;
	if (event == "sleep") kind = SimulationAudioEvent::Sleep;
	else if (event == "wake") kind = SimulationAudioEvent::Wake;
	else if (event == "stop") kind = SimulationAudioEvent::Stop;
	else if (event == "remove") kind = SimulationAudioEvent::Remove;
	else reader.fail("Expected simulation event start, sleep, wake, stop, or remove.");
	return {kind, reader.identifier("a spatial object ID")};
}

RegionAudioTrigger readRegionTrigger(AudioEventSourceReader &reader, bool is_camera)
{
	RegionAudioTrigger region;
	if (!is_camera) region.object_id = reader.identifier("a spatial object ID");
	const std::string transition = reader.identifier("enter or exit");
	if (transition == "exit") region.transition = AudioRegionTransition::Exit;
	else if (transition != "enter") reader.fail("Expected region transition enter or exit.");
	region.center.x = reader.number<float>("region X coordinate");
	region.center.y = reader.number<float>("region Y coordinate");
	region.center.z = reader.number<float>("region Z coordinate");
	region.radius = reader.number<float>("region radius");
	if (region.radius <= 0.0f) reader.fail("Region radius must be positive.");
	return region;
}

AudioEventTrigger readTrigger(AudioEventSourceReader &reader)
{
	const std::string kind = reader.identifier("an audio trigger");
	reader.expect('(');
	AudioEventTrigger trigger;
	if (kind == "time") {
		const double seconds = reader.number<double>("time in seconds");
		if (seconds < 0.0) reader.fail("Audio cue time must be nonnegative.");
		trigger = TimeAudioTrigger{seconds};
	} else if (kind == "simulation") {
		trigger = readSimulationTrigger(reader);
	} else if (kind == "collision") {
		CollisionAudioTrigger collision;
		collision.first_object_id = reader.identifier("the first spatial object ID");
		collision.second_object_id = reader.identifier("the second spatial object ID");
		if (collision.first_object_id == collision.second_object_id)
			reader.fail("Collision triggers require two distinct spatial object IDs.");
		if (reader.take(')')) return collision;
		collision.minimum_impulse = reader.number<float>("minimum collision impulse");
		if (collision.minimum_impulse < 0.0f) reader.fail("Minimum collision impulse must be nonnegative.");
		trigger = std::move(collision);
	} else if (kind == "camera" || kind == "position") {
		trigger = readRegionTrigger(reader, kind == "camera");
	} else {
		reader.fail("Expected audio trigger time, simulation, collision, camera, or position.");
	}
	reader.expect(')');
	return trigger;
}

} // namespace

AudioEventDeclarationResult AudioEventDeclarationParser::parse(std::string_view line) const
{
	AudioEventSourceReader reader(line);
	try {
		if (reader.identifier("audioEvent") != keyword) reader.fail("Expected audioEvent.");
		reader.expect('(');
		AudioEventDefinition definition;
		definition.id = reader.identifier("an audio event ID");
		definition.trigger = readTrigger(reader);
		if (reader.identifier("speak") != "speak") reader.fail("Expected speak(\"text\" [speakerId]).");
		reader.expect('(');
		definition.speech.text = reader.speech();
		if (!reader.take(')')) {
			definition.speech.speaker_id = reader.number<int>("speaker ID");
			if (definition.speech.speaker_id < 0) reader.fail("Speaker ID must be nonnegative.");
			reader.expect(')');
		}
		reader.expect(')');
		if (!reader.atEnd()) reader.fail("Unexpected text after audioEvent declaration.");
		return {std::move(definition), 0, {}};
	} catch (const std::invalid_argument &error) {
		return {std::nullopt, reader.column(), error.what()};
	}
}
