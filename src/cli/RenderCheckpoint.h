#pragma once

#include "RenderFiles.h"

#include <cstdint>
#include <optional>

class RenderCheckpoint {
public:
	RenderCheckpoint(const std::filesystem::path &directory, bool resume,
	                 const Json::Value &job, std::string_view grammar);

	const std::filesystem::path &directory() const { return directory_; }
	std::optional<Json::Value> restoreFrame(std::uint32_t frame, int width, int height, bool layers,
	                                      bool overlay = false);

private:
	std::filesystem::path directory_;
	render_files::FileLease lease_;
};
