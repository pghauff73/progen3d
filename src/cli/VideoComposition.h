#pragma once

#include <json/json.h>

#include <filesystem>

class VideoEdit;

Json::Value compose_video(const VideoEdit &edit, const std::filesystem::path &video_path);
