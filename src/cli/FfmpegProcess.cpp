#include "FfmpegProcess.h"

#include "AppPaths.h"

#include <windows.h>

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace {

namespace fs = std::filesystem;

class ProcessHandle {
public:
	explicit ProcessHandle(HANDLE handle) : handle_(handle) {}
	~ProcessHandle()
	{
		if (handle_ != nullptr && handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_);
	}
	ProcessHandle(const ProcessHandle &) = delete;
	ProcessHandle &operator=(const ProcessHandle &) = delete;
	HANDLE get() const { return handle_; }

private:
	HANDLE handle_;
};

fs::path find_ffmpeg()
{
	if (const wchar_t *configured = _wgetenv(L"PROGEN3D_FFMPEG"); configured && *configured) {
		const auto path = fs::absolute(fs::path(configured));
		if (!fs::is_regular_file(path)) {
			throw std::runtime_error("PROGEN3D_FFMPEG does not point to a file: " + path.u8string());
		}
		return path;
	}

	const auto adjacent = progen3d_app_dir() / "ffmpeg.exe";
	if (fs::is_regular_file(adjacent)) return adjacent;

	if (const wchar_t *search_path = _wgetenv(L"PATH"); search_path && *search_path) {
		std::wstring buffer(32768, L'\0');
		const DWORD length = SearchPathW(search_path, L"ffmpeg.exe", nullptr,
		                                static_cast<DWORD>(buffer.size()), buffer.data(), nullptr);
		if (length && length < buffer.size()) {
			buffer.resize(length);
			return fs::path(buffer);
		}
	}
	throw std::runtime_error("FFmpeg was not found. Set PROGEN3D_FFMPEG to ffmpeg.exe or put FFmpeg on PATH.");
}

std::wstring quote_argument(const std::wstring &value)
{
	std::wstring result = L"\"";
	std::size_t slashes = 0;
	for (const wchar_t character : value) {
		if (character == L'\\') {
			++slashes;
			continue;
		}
		result.append(character == L'"' ? 2 * slashes + 1 : slashes, L'\\');
		result += character;
		slashes = 0;
	}
	result.append(2 * slashes, L'\\');
	result += L'"';
	return result;
}

std::string log_tail(const fs::path &path)
{
	std::ifstream log(path, std::ios::binary | std::ios::ate);
	if (!log) return {};
	const auto size = log.tellg();
	if (size > 8192) log.seekg(-8192, std::ios::end);
	else log.seekg(0);
	return {std::istreambuf_iterator<char>(log), std::istreambuf_iterator<char>()};
}

std::system_error windows_error(const char *operation)
{
	return std::system_error(static_cast<int>(GetLastError()), std::system_category(), operation);
}

}

FfmpegProcess::FfmpegProcess(fs::path directory)
	: executable_(find_ffmpeg()), directory_(std::move(directory))
{
}

void FfmpegProcess::run(const std::vector<std::wstring> &arguments, const char *operation) const
{
	std::wstring command = quote_argument(executable_.wstring()) + L" -hide_banner -nostdin -loglevel error -n";
	for (const auto &argument : arguments) {
		command += L' ';
		command += quote_argument(argument);
	}
	if (command.size() >= 32767) throw std::runtime_error("Too many FFmpeg inputs for one Windows command.");

	const auto log_path = directory_ / "ffmpeg.log";
	DWORD exit_status = 0;
	{
		SECURITY_ATTRIBUTES attributes{sizeof(attributes), nullptr, TRUE};
		ProcessHandle log(CreateFileW(log_path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &attributes,
		                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
		if (log.get() == INVALID_HANDLE_VALUE) throw windows_error("Opening the FFmpeg log");
		ProcessHandle input(CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &attributes,
		                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
		if (input.get() == INVALID_HANDLE_VALUE) throw windows_error("Opening FFmpeg input");

		STARTUPINFOW startup{};
		startup.cb = sizeof(startup);
		startup.dwFlags = STARTF_USESTDHANDLES;
		startup.hStdInput = input.get();
		startup.hStdOutput = log.get();
		startup.hStdError = log.get();
		PROCESS_INFORMATION information{};
		if (!CreateProcessW(executable_.c_str(), command.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW,
		                    nullptr, directory_.c_str(), &startup, &information)) {
			throw windows_error("Starting FFmpeg");
		}
		ProcessHandle process(information.hProcess);
		ProcessHandle thread(information.hThread);
		if (WaitForSingleObject(process.get(), INFINITE) != WAIT_OBJECT_0 ||
		    !GetExitCodeProcess(process.get(), &exit_status)) {
			throw windows_error("Waiting for FFmpeg");
		}
	}
	if (exit_status != 0) {
		throw std::runtime_error(std::string("FFmpeg could not ") + operation + " (exit " +
		                         std::to_string(exit_status) + ").\n" + log_tail(log_path));
	}
}
