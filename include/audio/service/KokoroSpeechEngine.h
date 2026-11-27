#pragma once

#include "audio/model/AudioEventDefinition.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

struct SpeechWaveform {
	std::vector<std::int16_t> samples;
	int sample_rate = 0;
};

class KokoroSpeechEngine {
public:
	explicit KokoroSpeechEngine(const std::filesystem::path &directory);
	~KokoroSpeechEngine();

	SpeechWaveform synthesize(const SpeechCue &speech,
	                          const std::function<bool()> &cancelled = {}) const;

private:
	class Runtime;
	std::unique_ptr<Runtime> runtime_;
};
