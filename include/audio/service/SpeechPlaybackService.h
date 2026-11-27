#pragma once

#include "audio/model/AudioEventDefinition.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class SpeechPlaybackService {
public:
	SpeechPlaybackService();
	~SpeechPlaybackService();

	void configure(const std::filesystem::path &runtime_directory);
	void prepare(const std::vector<AudioEventDefinition> &definitions);
	void play(const SpeechCue &speech);
	void cancelPlayback();
	std::string status() const;

private:
	class PlaybackSession;
	std::unique_ptr<PlaybackSession> session_;
};
