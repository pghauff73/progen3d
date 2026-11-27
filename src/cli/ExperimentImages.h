#pragma once

#include "editor/model/PreviewImage.h"

#include <filesystem>
#include <string>
#include <vector>

PreviewImage read_experiment_image(const std::filesystem::path &path);
void write_experiment_png(const std::filesystem::path &path, const PreviewImage &image);

class ExperimentOutput {
public:
	explicit ExperimentOutput(const std::filesystem::path &destination);
	~ExperimentOutput();
	ExperimentOutput(const ExperimentOutput &) = delete;
	ExperimentOutput &operator=(const ExperimentOutput &) = delete;

	const std::filesystem::path &staging() const { return staging_; }
	const std::filesystem::path &destination() const { return destination_; }
	void publish();

private:
	std::filesystem::path destination_;
	std::filesystem::path staging_;
};

class ExperimentContactSheet {
public:
	explicit ExperimentContactSheet(std::filesystem::path directory);
	void add(std::string label, const PreviewImage &image = {});
	std::vector<std::string> finish();

private:
	struct Cell {
		std::string label;
		PreviewImage thumbnail;
	};
	void writePage();

	std::filesystem::path directory_;
	std::vector<Cell> page_;
	std::vector<std::string> files_;
};
