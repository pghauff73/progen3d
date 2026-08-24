#pragma once

#include <filesystem>
#include <string>

class ApplicationPathLocator
{
public:
	virtual ~ApplicationPathLocator() = default;
	virtual void initialize(const char *argv0) = 0;
	virtual std::filesystem::path applicationDirectory() = 0;
	virtual std::filesystem::path resourcePath(const std::filesystem::path &relative_path) = 0;
	virtual std::filesystem::path stateDirectory() = 0;
	virtual std::filesystem::path statePath(const std::string &filename) = 0;
};

class ProgenyApplicationPathLocator : public ApplicationPathLocator
{
public:
	void initialize(const char *argv0) override;
	std::filesystem::path applicationDirectory() override;
	std::filesystem::path resourcePath(const std::filesystem::path &relative_path) override;
	std::filesystem::path stateDirectory() override;
	std::filesystem::path statePath(const std::string &filename) override;
};

ApplicationPathLocator &progen3d_path_locator();

void progen3d_initialize_paths(const char *argv0);

std::filesystem::path progen3d_app_dir();
std::filesystem::path progen3d_resource_path(const std::filesystem::path &relative_path);
std::filesystem::path progen3d_state_dir();
std::filesystem::path progen3d_state_path(const std::string &filename);
