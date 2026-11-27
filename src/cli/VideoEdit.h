#pragma once

#include "audio/model/AudioEventDefinition.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct EditCut {
	std::filesystem::path sequence;
	std::uint32_t start_frame;
	std::uint32_t end_frame;
	std::uint32_t output_start_frame;
};

struct EditText {
	std::uint32_t start_frame;
	std::uint32_t end_frame;
	std::string text;
};

struct EditNarration {
	double start_seconds;
	double end_seconds;
	double gain;
	std::optional<std::filesystem::path> wav;
	SpeechCue speech;
	std::string id;
	std::string trigger;
};

class VideoEdit {
public:
	explicit VideoEdit(const std::filesystem::path &manifest);

	double duration() const { return static_cast<double>(frames.size()) / fps; }

	int fps = 0;
	int width = 0;
	int height = 0;
	std::vector<std::filesystem::path> frames;
	std::vector<EditCut> cuts;
	std::vector<EditText> titles;
	std::vector<EditText> captions;
	std::vector<EditNarration> narration;
};
