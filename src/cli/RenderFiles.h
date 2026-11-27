#pragma once

#include <json/json.h>
#include <windows.h>

#include <filesystem>
#include <string>
#include <string_view>

namespace render_files {

enum class LockWait { wait, try_once };

class FileLease {
public:
	explicit FileLease(const std::filesystem::path &path, LockWait wait = LockWait::wait);
	~FileLease();
	FileLease(const FileLease &) = delete;
	FileLease &operator=(const FileLease &) = delete;

	bool acquired() const { return file_ != INVALID_HANDLE_VALUE; }

private:
	HANDLE file_ = INVALID_HANDLE_VALUE;
};

class AtomicFile {
public:
	explicit AtomicFile(const std::filesystem::path &destination);
	~AtomicFile();
	AtomicFile(const AtomicFile &) = delete;
	AtomicFile &operator=(const AtomicFile &) = delete;

	const std::filesystem::path &staging() const { return staging_; }
	void publish();

private:
	std::filesystem::path destination_;
	std::filesystem::path staging_;
};

std::wstring unique_suffix();
void move_into_place(const std::filesystem::path &source, const std::filesystem::path &destination);
void flush_file(const std::filesystem::path &path);
void discard_abandoned_writes(const std::filesystem::path &destination);

std::string read_text(const std::filesystem::path &path);
void write_text(const std::filesystem::path &path, std::string_view text);
void write_json(const std::filesystem::path &path, const Json::Value &value);

std::string sha256_text(std::string_view text);
std::string sha256_file(const std::filesystem::path &path);

}
