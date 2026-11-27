#pragma once

#include <filesystem>
#include <string>
#include <vector>

class FfmpegProcess {
public:
	explicit FfmpegProcess(std::filesystem::path directory);

	void run(const std::vector<std::wstring> &arguments, const char *operation) const;

private:
	std::filesystem::path executable_;
	std::filesystem::path directory_;
};
