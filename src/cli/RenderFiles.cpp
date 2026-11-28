#include "RenderFiles.h"

#include <openssl/evp.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <system_error>

namespace render_files {
namespace {

namespace fs = std::filesystem;

std::system_error file_error(DWORD status, const char *operation, const fs::path &path)
{
	return std::system_error(static_cast<int>(status), std::system_category(),
	                         std::string(operation) + " " + path.u8string());
}

void move_file(const fs::path &source, const fs::path &destination, DWORD flags)
{
	if (!MoveFileExW(source.c_str(), destination.c_str(), flags | MOVEFILE_WRITE_THROUGH)) {
		throw file_error(GetLastError(), "Cannot publish", destination);
	}
}

class Sha256Digest {
public:
	Sha256Digest()
	{
		if (!digest_ || EVP_DigestInit_ex(digest_.get(), EVP_sha256(), nullptr) != 1) {
			throw std::runtime_error("Cannot initialize the SHA-256 checksum.");
		}
	}

	void add(std::string_view bytes)
	{
		if (EVP_DigestUpdate(digest_.get(), bytes.data(), bytes.size()) != 1) {
			throw std::runtime_error("Cannot update the SHA-256 checksum.");
		}
	}

	std::string finish()
	{
		std::array<unsigned char, EVP_MAX_MD_SIZE> bytes{};
		unsigned int size = 0;
		if (EVP_DigestFinal_ex(digest_.get(), bytes.data(), &size) != 1) {
			throw std::runtime_error("Cannot finish the SHA-256 checksum.");
		}
		constexpr char hex[] = "0123456789abcdef";
		std::string result;
		result.reserve(size * 2);
		for (unsigned int index = 0; index < size; ++index) {
			result += hex[bytes[index] >> 4];
			result += hex[bytes[index] & 15];
		}
		return result;
	}

private:
	std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> digest_{EVP_MD_CTX_new(), EVP_MD_CTX_free};
};

class ChecksumFileBuffer : public std::streambuf
{
public:
	explicit ChecksumFileBuffer(const fs::path &path)
	{
		if (!file_.open(path, std::ios::out | std::ios::binary)) {
			throw std::runtime_error("Cannot open output " + path.u8string());
		}
	}

	std::string finish(const fs::path &path)
	{
		if (!file_.close()) throw std::runtime_error("Cannot close output " + path.u8string());
		flush_file(path);
		return digest_.finish();
	}

protected:
	std::streamsize xsputn(const char *bytes, std::streamsize count) override
	{
		const auto written = file_.sputn(bytes, count);
		if (written > 0) digest_.add({bytes, static_cast<std::size_t>(written)});
		return written;
	}

	int_type overflow(int_type value) override
	{
		if (traits_type::eq_int_type(value, traits_type::eof())) return traits_type::not_eof(value);
		const char byte = traits_type::to_char_type(value);
		return xsputn(&byte, 1) == 1 ? value : traits_type::eof();
	}

	int sync() override { return file_.pubsync(); }

private:
	std::filebuf file_;
	Sha256Digest digest_;
};

}

FileLease::FileLease(const fs::path &path, LockWait wait)
{
	file_ = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
	                    nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file_ == INVALID_HANDLE_VALUE) throw file_error(GetLastError(), "Cannot open lock", path);
	OVERLAPPED region{};
	const DWORD flags = LOCKFILE_EXCLUSIVE_LOCK | (wait == LockWait::try_once ? LOCKFILE_FAIL_IMMEDIATELY : 0u);
	if (!LockFileEx(file_, flags, 0, 1, 0, &region)) {
		const DWORD status = GetLastError();
		CloseHandle(file_);
		file_ = INVALID_HANDLE_VALUE;
		if (wait == LockWait::wait || status != ERROR_LOCK_VIOLATION) {
			throw file_error(status, "Cannot acquire lock", path);
		}
	}
}

FileLease::~FileLease()
{
	if (acquired()) {
		OVERLAPPED region{};
		UnlockFileEx(file_, 0, 1, 0, &region);
		CloseHandle(file_);
	}
}

std::wstring unique_suffix()
{
	static std::atomic<unsigned int> counter{0};
	return std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()) + L"-" +
	       std::to_wstring(counter.fetch_add(1, std::memory_order_relaxed));
}

AtomicFile::AtomicFile(const fs::path &destination)
	: destination_(destination),
	  staging_(destination.parent_path() / (L"." + destination.filename().wstring() + L"-" + unique_suffix() + L".tmp"))
{
	const HANDLE file = CreateFileW(staging_.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
	                                FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE) throw file_error(GetLastError(), "Cannot stage", destination);
	CloseHandle(file);
}

AtomicFile::~AtomicFile()
{
	if (!staging_.empty()) {
		std::error_code ignored;
		fs::remove(staging_, ignored);
	}
}

void AtomicFile::publish()
{
	flush_file(staging_);
	move_file(staging_, destination_, MOVEFILE_REPLACE_EXISTING);
	staging_.clear();
}

void move_into_place(const fs::path &source, const fs::path &destination)
{
	move_file(source, destination, 0);
}

void flush_file(const fs::path &path)
{
	const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
	                                FILE_ATTRIBUTE_NORMAL, nullptr);
	if (file == INVALID_HANDLE_VALUE) throw file_error(GetLastError(), "Cannot open for flushing", path);
	const BOOL flushed = FlushFileBuffers(file);
	const DWORD status = flushed ? ERROR_SUCCESS : GetLastError();
	CloseHandle(file);
	if (!flushed) throw file_error(status, "Cannot flush", path);
}

void discard_abandoned_writes(const fs::path &destination)
{
	const std::wstring prefix = L"." + destination.filename().wstring() + L"-";
	for (const auto &child : fs::directory_iterator(destination.parent_path())) {
		const auto name = child.path().filename().wstring();
		if (name.size() <= prefix.size() + 4 || name.compare(0, prefix.size(), prefix) != 0 ||
		    child.path().extension() != L".tmp" || !fs::is_regular_file(child.symlink_status())) continue;
		const auto suffix = name.substr(prefix.size(), name.size() - prefix.size() - 4);
		if (std::all_of(suffix.begin(), suffix.end(), [](wchar_t ch) { return (ch >= L'0' && ch <= L'9') || ch == L'-'; })) {
			std::error_code ignored;
			fs::remove(child.path(), ignored);
		}
	}
}

std::string read_text(const fs::path &path)
{
	std::ifstream input(path, std::ios::binary);
	if (!input) throw std::runtime_error("Cannot open " + path.u8string());
	input.exceptions(std::ios::badbit);
	return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void write_text(const fs::path &path, std::string_view text)
{
	AtomicFile pending(path);
	std::ofstream output(pending.staging(), std::ios::binary);
	output.exceptions(std::ios::badbit | std::ios::failbit);
	output.write(text.data(), static_cast<std::streamsize>(text.size()));
	output.close();
	pending.publish();
}

void write_json(const fs::path &path, const Json::Value &value)
{
	Json::StreamWriterBuilder writer;
	writer["indentation"] = "  ";
	write_text(path, Json::writeString(writer, value) + "\n");
}

std::string write_checksummed_file(const fs::path &path,
                                   const std::function<void(std::ostream &)> &write)
{
	ChecksumFileBuffer buffer(path);
	std::ostream output(&buffer);
	output.exceptions(std::ios::failbit | std::ios::badbit);
	write(output);
	return buffer.finish(path);
}

std::string sha256_text(std::string_view text)
{
	Sha256Digest digest;
	digest.add(text);
	return digest.finish();
}

std::string sha256_file(const fs::path &path)
{
	std::ifstream input(path, std::ios::binary);
	if (!input) throw std::runtime_error("Cannot read checksum input " + path.u8string());
	input.exceptions(std::ios::badbit);
	Sha256Digest digest;
	std::array<char, 64 * 1024> buffer{};
	do {
		input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
		digest.add(std::string_view(buffer.data(), static_cast<std::size_t>(input.gcount())));
	} while (input);
	if (!input.eof()) throw std::runtime_error("Cannot read checksum input " + path.u8string());
	return digest.finish();
}

}
