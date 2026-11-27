#pragma once

#include "SceneLayerCapture.h"
#include "editor/model/PreviewImage.h"

#include <json/json.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

struct RenderFrameLayers {
	PreviewImage sharp_color;
	CapturedSceneLayers geometry;
	std::vector<SceneMaskLabel> labels;
	std::vector<std::uint8_t> foreground_coverage;
};

struct RenderFrameOverlay {
	// Interleaved linear RGB, in the same top-down order as preview pixels.
	std::vector<float> emission_rgb;
	Json::Value records;
};

struct RenderedFrame {
	PreviewImage color;
	std::optional<RenderFrameLayers> layers;
	std::optional<RenderFrameOverlay> overlay;
};

inline constexpr char frame_emission_name[] = "emission.pfm";
inline constexpr char frame_effects_name[] = "effects.json";

void publish_layered_frame(const std::filesystem::path &output_directory,
                           const RenderedFrame &frame, Json::Value &report);

std::filesystem::path publish_sequence_frame(const std::filesystem::path &output_directory,
                                             std::uint32_t frame_number, const RenderedFrame &frame,
                                             Json::Value metadata);

std::filesystem::path sequence_frame_directory(const std::filesystem::path &output_directory,
                                               std::uint32_t frame_number);
std::optional<Json::Value> verify_sequence_frame(const std::filesystem::path &directory,
                                               std::uint32_t frame_number, int width, int height, bool layers,
                                               bool overlay = false);
void discard_abandoned_frame_staging(const std::filesystem::path &directory);
