#include "BackendApiClient.h"
#include "AppPaths.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <curl/curl.h>

namespace {

struct HttpResponse {
	long status_code = 0;
	std::string body;
	std::string error;
};

struct MultipartField {
	std::string name;
	std::string value;
};

using Clock = std::chrono::steady_clock;

constexpr long kApiTimeoutSeconds = 8L;
constexpr long kConnectTimeoutSeconds = 3L;
constexpr long kMultipartTimeoutSeconds = 20L;
constexpr std::chrono::seconds kFileListCacheTtl(10);
constexpr std::chrono::seconds kFileCacheTtl(30);

struct CachedFileList {
	std::vector<BackendFileSummary> files;
	Clock::time_point updated_at{};
	bool valid = false;
};

struct CachedFileRecord {
	BackendFileRecord file;
	Clock::time_point updated_at{};
	bool valid = false;
};

std::mutex g_backend_cache_mutex;
CachedFileList g_cached_file_list;
std::unordered_map<std::string, CachedFileRecord> g_cached_files;

std::mutex g_inflight_mutex;
bool g_list_request_running = false;
std::vector<std::pair<std::shared_ptr<std::atomic<bool>>, BackendListFilesCallback>>
	g_pending_list_callbacks;
std::unordered_map<std::string,
                   std::vector<std::pair<std::shared_ptr<std::atomic<bool>>,
                                         BackendGetFileCallback>>>
	g_pending_file_callbacks;

std::filesystem::path backend_cookie_jar_path()
{
	return progen3d_state_path("backend_session_cookies.txt");
}

std::string trim_copy(const std::string &value)
{
	std::size_t start = 0;
	while (start < value.size() &&
	       std::isspace(static_cast<unsigned char>(value[start])) != 0) {
		++start;
	}
	std::size_t end = value.size();
	while (end > start &&
	       std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
		--end;
	}
	return value.substr(start, end - start);
}

bool backend_url_looks_https(const std::string &url)
{
	const std::string normalized = trim_copy(url);
	return normalized.rfind("https://", 0) == 0;
}

bool cache_is_fresh(const Clock::time_point &timestamp, std::chrono::seconds ttl)
{
	return timestamp.time_since_epoch().count() != 0 &&
	       (Clock::now() - timestamp) < ttl;
}

void store_cached_file_list(const std::vector<BackendFileSummary> &files)
{
	std::lock_guard<std::mutex> lock(g_backend_cache_mutex);
	g_cached_file_list.files = files;
	g_cached_file_list.updated_at = Clock::now();
	g_cached_file_list.valid = true;
}

void invalidate_cached_file_list()
{
	std::lock_guard<std::mutex> lock(g_backend_cache_mutex);
	g_cached_file_list = CachedFileList{};
}

void store_cached_file(const BackendFileRecord &file)
{
	const std::string file_id = trim_copy(file.id);
	if (file_id.empty()) {
		return;
	}
	std::lock_guard<std::mutex> lock(g_backend_cache_mutex);
	CachedFileRecord &entry = g_cached_files[file_id];
	entry.file = file;
	entry.updated_at = Clock::now();
	entry.valid = true;
}

void erase_cached_file(const std::string &file_id)
{
	const std::string normalized_id = trim_copy(file_id);
	if (normalized_id.empty()) {
		return;
	}
	std::lock_guard<std::mutex> lock(g_backend_cache_mutex);
	g_cached_files.erase(normalized_id);
}

bool load_cached_file_list(std::vector<BackendFileSummary> *files)
{
	if (files == nullptr) {
		return false;
	}
	std::lock_guard<std::mutex> lock(g_backend_cache_mutex);
	if (!g_cached_file_list.valid ||
	    !cache_is_fresh(g_cached_file_list.updated_at, kFileListCacheTtl)) {
		return false;
	}
	*files = g_cached_file_list.files;
	return true;
}

bool load_cached_file(const std::string &file_id, BackendFileRecord *file)
{
	if (file == nullptr) {
		return false;
	}
	const std::string normalized_id = trim_copy(file_id);
	if (normalized_id.empty()) {
		return false;
	}
	std::lock_guard<std::mutex> lock(g_backend_cache_mutex);
	const auto it = g_cached_files.find(normalized_id);
	if (it == g_cached_files.end()) {
		return false;
	}
	if (!it->second.valid || !cache_is_fresh(it->second.updated_at, kFileCacheTtl)) {
		return false;
	}
	*file = it->second.file;
	return true;
}

void clear_backend_cache_internal()
{
	std::lock_guard<std::mutex> lock(g_backend_cache_mutex);
	g_cached_file_list = CachedFileList{};
	g_cached_files.clear();
}

std::string json_escape(const std::string &value)
{
	std::string escaped;
	escaped.reserve(value.size() + 8);
	for (char ch : value) {
		switch (ch) {
		case '\\':
			escaped += "\\\\";
			break;
		case '"':
			escaped += "\\\"";
			break;
		case '\n':
			escaped += "\\n";
			break;
		case '\r':
			escaped += "\\r";
			break;
		case '\t':
			escaped += "\\t";
			break;
		default:
			escaped.push_back(ch);
			break;
		}
	}
	return escaped;
}

std::string url_encode(const std::string &value)
{
	std::string init_error;
	initialize_firebase_auth_support(&init_error);

	CURL *curl = curl_easy_init();
	if (curl == nullptr) {
		return "";
	}

	char *escaped = curl_easy_escape(curl, value.c_str(), static_cast<int>(value.size()));
	if (escaped == nullptr) {
		curl_easy_cleanup(curl);
		return "";
	}

	std::string encoded(escaped);
	curl_free(escaped);
	curl_easy_cleanup(curl);
	return encoded;
}

void skip_json_whitespace(const std::string &text, std::size_t *index)
{
	if (index == nullptr) {
		return;
	}
	while (*index < text.size() &&
	       std::isspace(static_cast<unsigned char>(text[*index])) != 0) {
		++(*index);
	}
}

bool parse_json_string_literal(const std::string &text,
                               std::size_t *index,
                               std::string *value)
{
	if (index == nullptr || value == nullptr || *index >= text.size() || text[*index] != '"') {
		return false;
	}
	++(*index);
	value->clear();
	bool escaped = false;
	while (*index < text.size()) {
		const char ch = text[*index];
		++(*index);
		if (escaped) {
			switch (ch) {
			case '\\':
			case '"':
			case '/':
				value->push_back(ch);
				break;
			case 'b':
				value->push_back('\b');
				break;
			case 'f':
				value->push_back('\f');
				break;
			case 'n':
				value->push_back('\n');
				break;
			case 'r':
				value->push_back('\r');
				break;
			case 't':
				value->push_back('\t');
				break;
			default:
				value->push_back(ch);
				break;
			}
			escaped = false;
			continue;
		}
		if (ch == '\\') {
			escaped = true;
			continue;
		}
		if (ch == '"') {
			return true;
		}
		value->push_back(ch);
	}
	return false;
}

std::size_t find_json_value_end(const std::string &text, std::size_t start)
{
	std::size_t index = start;
	skip_json_whitespace(text, &index);
	if (index >= text.size()) {
		return std::string::npos;
	}
	if (text[index] == '"') {
		std::string ignored;
		return parse_json_string_literal(text, &index, &ignored) ? index : std::string::npos;
	}
	if (text[index] == '{' || text[index] == '[') {
		const char opening = text[index];
		const char closing = opening == '{' ? '}' : ']';
		int depth = 0;
		bool in_string = false;
		bool escaped = false;
		for (; index < text.size(); ++index) {
			const char ch = text[index];
			if (in_string) {
				if (escaped) {
					escaped = false;
				} else if (ch == '\\') {
					escaped = true;
				} else if (ch == '"') {
					in_string = false;
				}
				continue;
			}
			if (ch == '"') {
				in_string = true;
				continue;
			}
			if (ch == opening) {
				++depth;
				continue;
			}
			if (ch == closing) {
				--depth;
				if (depth == 0) {
					return index + 1;
				}
			}
		}
		return std::string::npos;
	}

	while (index < text.size()) {
		const char ch = text[index];
		if (ch == ',' || ch == '}' || ch == ']') {
			break;
		}
		++index;
	}
	return index;
}

bool extract_json_value_for_key(const std::string &json,
                                const std::string &key,
                                std::string *value)
{
	if (value == nullptr) {
		return false;
	}

	std::size_t index = 0;
	skip_json_whitespace(json, &index);
	if (index >= json.size() || json[index] != '{') {
		return false;
	}
	++index;

	while (index < json.size()) {
		skip_json_whitespace(json, &index);
		if (index >= json.size() || json[index] == '}') {
			break;
		}

		std::string current_key;
		if (!parse_json_string_literal(json, &index, &current_key)) {
			return false;
		}
		skip_json_whitespace(json, &index);
		if (index >= json.size() || json[index] != ':') {
			return false;
		}
		++index;
		const std::size_t value_start = index;
		const std::size_t value_end = find_json_value_end(json, value_start);
		if (value_end == std::string::npos) {
			return false;
		}
		if (current_key == key) {
			*value = json.substr(value_start, value_end - value_start);
			return true;
		}
		index = value_end;
		skip_json_whitespace(json, &index);
		if (index < json.size() && json[index] == ',') {
			++index;
		}
	}

	return false;
}

std::string parse_json_string_value(const std::string &raw)
{
	std::size_t index = 0;
	skip_json_whitespace(raw, &index);
	std::string value;
	if (!parse_json_string_literal(raw, &index, &value)) {
		return "";
	}
	return value;
}

bool parse_json_bool_value(const std::string &raw, bool fallback = false)
{
	const std::string normalized = trim_copy(raw);
	if (normalized == "true" || normalized == "1") {
		return true;
	}
	if (normalized == "false" || normalized == "0" || normalized == "null") {
		return false;
	}
	return fallback;
}

int parse_json_int_value(const std::string &raw, int fallback = 0)
{
	const std::string normalized = trim_copy(raw);
	if (normalized.empty() || normalized == "null") {
		return fallback;
	}

	try {
		return std::stoi(normalized);
	} catch (...) {
		return fallback;
	}
}

float parse_json_float_value(const std::string &raw, float fallback = 0.0f)
{
	const std::string normalized = trim_copy(raw);
	if (normalized.empty() || normalized == "null") {
		return fallback;
	}

	try {
		return std::stof(normalized);
	} catch (...) {
		return fallback;
	}
}

std::vector<std::string> extract_top_level_json_objects(const std::string &json_array)
{
	std::vector<std::string> objects;
	std::size_t index = 0;
	skip_json_whitespace(json_array, &index);
	if (index >= json_array.size() || json_array[index] != '[') {
		return objects;
	}
	++index;

	while (index < json_array.size()) {
		skip_json_whitespace(json_array, &index);
		if (index >= json_array.size() || json_array[index] == ']') {
			break;
		}
		if (json_array[index] != '{') {
			++index;
			continue;
		}
		const std::size_t object_start = index;
		const std::size_t object_end = find_json_value_end(json_array, index);
		if (object_end == std::string::npos || object_end <= object_start) {
			break;
		}
		objects.push_back(json_array.substr(object_start, object_end - object_start));
		index = object_end;
		skip_json_whitespace(json_array, &index);
		if (index < json_array.size() && json_array[index] == ',') {
			++index;
		}
	}

	return objects;
}

std::vector<std::string> extract_top_level_json_values(const std::string &json_array)
{
	std::vector<std::string> values;
	std::size_t index = 0;
	skip_json_whitespace(json_array, &index);
	if (index >= json_array.size() || json_array[index] != '[') {
		return values;
	}
	++index;

	while (index < json_array.size()) {
		skip_json_whitespace(json_array, &index);
		if (index >= json_array.size() || json_array[index] == ']') {
			break;
		}

		const std::size_t value_start = index;
		const std::size_t value_end = find_json_value_end(json_array, value_start);
		if (value_end == std::string::npos || value_end <= value_start) {
			break;
		}
		values.push_back(json_array.substr(value_start, value_end - value_start));
		index = value_end;
		skip_json_whitespace(json_array, &index);
		if (index < json_array.size() && json_array[index] == ',') {
			++index;
		}
	}

	return values;
}

std::vector<std::string> parse_json_string_array(const std::string &json_array)
{
	std::vector<std::string> values;
	for (const std::string &raw_value : extract_top_level_json_values(json_array)) {
		const std::string parsed = parse_json_string_value(raw_value);
		if (!parsed.empty()) {
			values.push_back(parsed);
		}
	}
	return values;
}

std::string url_decode_component(const std::string &value)
{
	std::string decoded;
	decoded.reserve(value.size());
	for (std::size_t index = 0; index < value.size(); ++index) {
		const char ch = value[index];
		if (ch == '%' && index + 2 < value.size()) {
			const auto decode_nibble = [](char digit) -> int {
				if (digit >= '0' && digit <= '9') {
					return digit - '0';
				}
				if (digit >= 'a' && digit <= 'f') {
					return 10 + (digit - 'a');
				}
				if (digit >= 'A' && digit <= 'F') {
					return 10 + (digit - 'A');
				}
				return -1;
			};
			const int hi = decode_nibble(value[index + 1]);
			const int lo = decode_nibble(value[index + 2]);
			if (hi >= 0 && lo >= 0) {
				decoded.push_back(static_cast<char>((hi << 4) | lo));
				index += 2;
				continue;
			}
		}
		if (ch == '+') {
			decoded.push_back(' ');
		} else {
			decoded.push_back(ch);
		}
	}
	return decoded;
}

void replace_all_inplace(std::string *text, const std::string &from, const std::string &to)
{
	if (text == nullptr || from.empty()) {
		return;
	}
	std::size_t position = 0;
	while ((position = text->find(from, position)) != std::string::npos) {
		text->replace(position, from.size(), to);
		position += to.size();
	}
}

std::string html_unescape(std::string value)
{
	replace_all_inplace(&value, "&amp;", "&");
	replace_all_inplace(&value, "&lt;", "<");
	replace_all_inplace(&value, "&gt;", ">");
	replace_all_inplace(&value, "&quot;", "\"");
	replace_all_inplace(&value, "&#39;", "'");
	replace_all_inplace(&value, "&#039;", "'");
	replace_all_inplace(&value, "&nbsp;", " ");
	return value;
}

std::string extract_between(const std::string &text,
                            const std::string &prefix,
                            const std::string &suffix)
{
	const std::size_t start = text.find(prefix);
	if (start == std::string::npos) {
		return "";
	}
	const std::size_t content_start = start + prefix.size();
	const std::size_t end = text.find(suffix, content_start);
	if (end == std::string::npos || end < content_start) {
		return "";
	}
	return text.substr(content_start, end - content_start);
}

bool parse_backend_file_list_from_html(const std::string &body,
                                       std::vector<BackendFileSummary> *files)
{
	if (files == nullptr) {
		return false;
	}

	files->clear();
	const bool looks_like_workspace_page =
		body.find("My grammar files") != std::string::npos ||
		body.find("No saved files yet") != std::string::npos ||
		body.find("file-card") != std::string::npos;
	if (!looks_like_workspace_page) {
		return false;
	}

	const std::string marker = "<article class=\"panel file-card\">";
	std::size_t position = 0;
	while ((position = body.find(marker, position)) != std::string::npos) {
		const std::size_t end = body.find("</article>", position);
		if (end == std::string::npos) {
			break;
		}

		const std::string block = body.substr(position, end - position);
		BackendFileSummary file;
		file.id = url_decode_component(html_unescape(
			extract_between(block, "href=\"editor.php?file=", "\"")));
		file.title = trim_copy(html_unescape(extract_between(block, "<h2>", "</h2>")));
		file.updated_at = trim_copy(html_unescape(
			extract_between(block, "<p class=\"muted\">Updated ", " UTC</p>")));
		file.is_published = block.find(">Published<") != std::string::npos;
		if (!file.id.empty()) {
			files->push_back(std::move(file));
		}

		position = end + std::string("</article>").size();
	}

	return true;
}

std::size_t curl_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	if (userdata == nullptr) {
		return 0;
	}
	std::string *buffer = static_cast<std::string *>(userdata);
	buffer->append(ptr, size * nmemb);
	return size * nmemb;
}

std::string response_error_message(const HttpResponse &response)
{
	std::string raw_error;
	const bool has_error = extract_json_value_for_key(response.body, "error", &raw_error);
	const std::string parsed_error = has_error ? parse_json_string_value(raw_error) : "";
	if (!parsed_error.empty()) {
		std::string message = parsed_error;
		std::string raw_detail;
		if (extract_json_value_for_key(response.body, "detail", &raw_detail)) {
			const std::string parsed_detail = parse_json_string_value(raw_detail);
			if (!parsed_detail.empty() && parsed_detail != parsed_error) {
				message += " Detail: " + parsed_detail;
			}
		}
		if (response.status_code >= 400) {
			message += " (HTTP " + std::to_string(response.status_code) + ")";
		}
		return message;
	}
	if (!response.error.empty()) {
		return response.error;
	}
	const std::string body = trim_copy(response.body);
	if (!body.empty()) {
		return body;
	}
	if (response.status_code > 0) {
		return "Backend request failed with HTTP " + std::to_string(response.status_code) + ".";
	}
	return "Backend request failed.";
}

bool backend_error_is_server_failure(const std::string &error)
{
	const std::string normalized = trim_copy(error);
	return normalized.empty() ||
	       normalized.find("HTTP 5") != std::string::npos ||
	       normalized.find("Unknown action") != std::string::npos;
}

bool backend_error_is_server_failure(const HttpResponse &response,
                                     const std::string &error)
{
	if (response.status_code >= 500) {
		return true;
	}
	if (response.status_code == 400 &&
	    response.body.find("Unknown action") != std::string::npos) {
		return true;
	}
	return backend_error_is_server_failure(error);
}

bool extract_script_json_assignment(const std::string &body,
                                    const std::string &assignment,
                                    std::string *json)
{
	if (json == nullptr) {
		return false;
	}

	const std::size_t assignment_pos = body.find(assignment);
	if (assignment_pos == std::string::npos) {
		return false;
	}

	std::size_t value_start = assignment_pos + assignment.size();
	skip_json_whitespace(body, &value_start);
	const std::size_t value_end = find_json_value_end(body, value_start);
	if (value_end == std::string::npos || value_end <= value_start) {
		return false;
	}

	*json = body.substr(value_start, value_end - value_start);
	return true;
}

bool perform_request(const std::string &method,
                     const std::string &url,
                     const std::string &body,
                     const std::vector<std::string> &headers,
                     HttpResponse *response)
{
	if (response == nullptr) {
		return false;
	}

	std::string init_error;
	if (!initialize_firebase_auth_support(&init_error)) {
		response->error = init_error.empty() ? "Failed to initialize libcurl." : init_error;
		return false;
	}

	CURL *curl = curl_easy_init();
	if (curl == nullptr) {
		response->error = "Unable to create libcurl request handle.";
		return false;
	}

	std::array<char, CURL_ERROR_SIZE> curl_error_buffer{};
	curl_error_buffer[0] = '\0';

	struct curl_slist *curl_headers = nullptr;
	for (const std::string &header : headers) {
		curl_headers = curl_slist_append(curl_headers, header.c_str());
	}

	response->status_code = 0;
	response->body.clear();
	response->error.clear();

	const std::filesystem::path cookie_path = backend_cookie_jar_path();
	if (cookie_path.has_parent_path()) {
		std::error_code create_error;
		std::filesystem::create_directories(cookie_path.parent_path(), create_error);
	}
	std::ofstream(cookie_path, std::ios::app).close();
	const std::string cookie_path_string = cookie_path.string();

	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error_buffer.data());
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response->body);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, kConnectTimeoutSeconds);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, kApiTimeoutSeconds);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Progen3d BackendApi/1.0");
	curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookie_path_string.c_str());
	curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookie_path_string.c_str());
	if (!firebase_auth_configure_curl_tls(curl, &response->error)) {
		curl_slist_free_all(curl_headers);
		curl_easy_cleanup(curl);
		return false;
	}

	if (method == "POST" || method == "PUT" || method == "PATCH") {
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
	}

	const CURLcode result = curl_easy_perform(curl);
	if (result != CURLE_OK) {
		response->error =
			firebase_auth_build_curl_error_message(curl, result, curl_error_buffer.data(), url);
		curl_slist_free_all(curl_headers);
		curl_easy_cleanup(curl);
		return false;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->status_code);
	curl_slist_free_all(curl_headers);
	curl_easy_cleanup(curl);
	return true;
}

bool perform_multipart_request(const std::string &url,
                               const std::vector<std::string> &headers,
                               const std::vector<MultipartField> &fields,
                               const std::string &file_field_name,
                               const std::string &file_path,
                               HttpResponse *response)
{
	if (response == nullptr) {
		return false;
	}

	std::string init_error;
	if (!initialize_firebase_auth_support(&init_error)) {
		response->error = init_error.empty() ? "Failed to initialize libcurl." : init_error;
		return false;
	}

	CURL *curl = curl_easy_init();
	if (curl == nullptr) {
		response->error = "Unable to create libcurl request handle.";
		return false;
	}

	std::array<char, CURL_ERROR_SIZE> curl_error_buffer{};
	curl_error_buffer[0] = '\0';

	struct curl_slist *curl_headers = nullptr;
	for (const std::string &header : headers) {
		curl_headers = curl_slist_append(curl_headers, header.c_str());
	}

	curl_mime *mime = curl_mime_init(curl);
	if (mime == nullptr) {
		response->error = "Unable to create libcurl multipart payload.";
		curl_slist_free_all(curl_headers);
		curl_easy_cleanup(curl);
		return false;
	}

	for (const MultipartField &field : fields) {
		curl_mimepart *part = curl_mime_addpart(mime);
		curl_mime_name(part, field.name.c_str());
		curl_mime_data(part, field.value.c_str(), CURL_ZERO_TERMINATED);
	}

	if (!trim_copy(file_field_name).empty() && !trim_copy(file_path).empty()) {
		curl_mimepart *part = curl_mime_addpart(mime);
		curl_mime_name(part, file_field_name.c_str());
		curl_mime_filedata(part, file_path.c_str());
	}

	response->status_code = 0;
	response->body.clear();
	response->error.clear();

	const std::filesystem::path cookie_path = backend_cookie_jar_path();
	if (cookie_path.has_parent_path()) {
		std::error_code create_error;
		std::filesystem::create_directories(cookie_path.parent_path(), create_error);
	}
	std::ofstream(cookie_path, std::ios::app).close();
	const std::string cookie_path_string = cookie_path.string();

	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error_buffer.data());
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
	curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response->body);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, kConnectTimeoutSeconds);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, kMultipartTimeoutSeconds);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Progen3d BackendApi/1.0");
	curl_easy_setopt(curl, CURLOPT_COOKIEFILE, cookie_path_string.c_str());
	curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookie_path_string.c_str());
	if (!firebase_auth_configure_curl_tls(curl, &response->error)) {
		curl_mime_free(mime);
		curl_slist_free_all(curl_headers);
		curl_easy_cleanup(curl);
		return false;
	}

	const CURLcode result = curl_easy_perform(curl);
	if (result != CURLE_OK) {
		response->error =
			firebase_auth_build_curl_error_message(curl, result, curl_error_buffer.data(), url);
		curl_mime_free(mime);
		curl_slist_free_all(curl_headers);
		curl_easy_cleanup(curl);
		return false;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->status_code);
	curl_mime_free(mime);
	curl_slist_free_all(curl_headers);
	curl_easy_cleanup(curl);
	return true;
}

bool ensure_backend_session(const FirebaseAuthConfig &config,
                            FirebaseAuthSession *session,
                            std::string *error)
{
	if (!config.valid()) {
		if (error != nullptr) {
			*error = "Firebase config is incomplete.";
		}
		return false;
	}
	if (trim_copy(config.backend_base_url).empty()) {
		if (error != nullptr) {
			*error = "Backend URL is not configured.";
		}
		return false;
	}
	if (!backend_url_looks_https(config.backend_base_url)) {
		if (error != nullptr) {
			*error = "Backend URL must use https:// for TLS verification.";
		}
		return false;
	}
	if (session == nullptr || !session->authenticated || session->refresh_token.empty()) {
		if (error != nullptr) {
			*error = "Sign in to Firebase before using the cloud backend.";
		}
		return false;
	}
	if (!session->id_token.empty()) {
		return true;
	}
	if (!firebase_auth_refresh(config, session, error)) {
		return false;
	}
	std::string save_error;
	save_firebase_auth_session(*session, &save_error);
	return true;
}

bool authorized_backend_request(const FirebaseAuthConfig &config,
                                FirebaseAuthSession *session,
                                const std::string &method,
                                const std::string &path_and_query,
                                const std::string &body,
                                HttpResponse *response,
                                std::string *error)
{
	if (!ensure_backend_session(config, session, error)) {
		return false;
	}

	std::vector<std::string> headers{
		"Accept: application/json",
		"Content-Type: application/json",
	};

	for (int attempt = 0; attempt < 2; ++attempt) {
		const std::string url = config.backend_base_url + path_and_query;
		const std::string authorization = "Authorization: Bearer " + session->id_token;
		std::vector<std::string> request_headers = headers;
		request_headers.push_back(authorization);

		HttpResponse next_response;
		if (!perform_request(method, url, body, request_headers, &next_response)) {
			if (error != nullptr) {
				*error = response_error_message(next_response);
			}
			return false;
		}

		if ((next_response.status_code == 401 || next_response.status_code == 403) &&
		    attempt == 0 &&
		    firebase_auth_refresh(config, session, error)) {
			std::string save_error;
			save_firebase_auth_session(*session, &save_error);
			continue;
		}

		*response = std::move(next_response);
		if (response->status_code >= 200 && response->status_code < 300) {
			return true;
		}
		if (error != nullptr) {
			*error = response_error_message(*response);
		}
		return false;
	}

	if (error != nullptr && error->empty()) {
		*error = "Backend request failed.";
	}
	return false;
}

bool authorized_backend_multipart_request(const FirebaseAuthConfig &config,
                                          FirebaseAuthSession *session,
                                          const std::string &path_and_query,
                                          const std::vector<MultipartField> &fields,
                                          const std::string &file_field_name,
                                          const std::string &file_path,
                                          HttpResponse *response,
                                          std::string *error)
{
	if (!ensure_backend_session(config, session, error)) {
		return false;
	}

	std::vector<std::string> headers{
		"Accept: application/json",
	};

	for (int attempt = 0; attempt < 2; ++attempt) {
		const std::string url = config.backend_base_url + path_and_query;
		const std::string authorization = "Authorization: Bearer " + session->id_token;
		std::vector<std::string> request_headers = headers;
		request_headers.push_back(authorization);

		HttpResponse next_response;
		if (!perform_multipart_request(url, request_headers, fields, file_field_name, file_path, &next_response)) {
			if (error != nullptr) {
				*error = response_error_message(next_response);
			}
			return false;
		}

		if ((next_response.status_code == 401 || next_response.status_code == 403) &&
		    attempt == 0 &&
		    firebase_auth_refresh(config, session, error)) {
			std::string save_error;
			save_firebase_auth_session(*session, &save_error);
			continue;
		}

		*response = std::move(next_response);
		if (response->status_code >= 200 && response->status_code < 300) {
			return true;
		}
		if (error != nullptr) {
			*error = response_error_message(*response);
		}
		return false;
	}

	if (error != nullptr && error->empty()) {
		*error = "Backend upload request failed.";
	}
	return false;
}

bool parse_backend_credit_summary(const std::string &body, BackendCreditSummary *credits)
{
	if (credits == nullptr) {
		return false;
	}
	*credits = BackendCreditSummary{};

	std::string raw_credits;
	if (!extract_json_value_for_key(body, "credits", &raw_credits)) {
		return false;
	}

	credits->loaded = true;
	std::string raw_value;
	if (extract_json_value_for_key(raw_credits, "balance", &raw_value)) {
		credits->balance = parse_json_int_value(raw_value);
	}
	if (extract_json_value_for_key(raw_credits, "available", &raw_value)) {
		credits->available = parse_json_int_value(raw_value, credits->balance);
	}
	if (extract_json_value_for_key(raw_credits, "granted_lifetime", &raw_value)) {
		credits->granted_lifetime = parse_json_int_value(raw_value);
	}
	if (extract_json_value_for_key(raw_credits, "spent_lifetime", &raw_value)) {
		credits->spent_lifetime = parse_json_int_value(raw_value);
	}
	if (extract_json_value_for_key(raw_credits, "reserved", &raw_value)) {
		credits->reserved = parse_json_int_value(raw_value);
	}
	if (extract_json_value_for_key(raw_credits, "plan", &raw_value)) {
		credits->plan = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(raw_credits, "updated_at", &raw_value)) {
		credits->updated_at = parse_json_string_value(raw_value);
	}
	return true;
}

bool parse_backend_user_profile(const std::string &body, BackendUserProfile *profile)
{
	if (profile == nullptr) {
		return false;
	}
	*profile = BackendUserProfile{};
	std::string raw_user;
	if (!extract_json_value_for_key(body, "user", &raw_user)) {
		return true;
	}
	profile->authenticated = true;
	std::string raw_value;
	if (extract_json_value_for_key(raw_user, "id", &raw_value)) {
		profile->id = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(raw_user, "username", &raw_value)) {
		profile->username = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(raw_user, "email", &raw_value)) {
		profile->email = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(raw_user, "role", &raw_value)) {
		profile->role = parse_json_string_value(raw_value);
	}

	std::string raw_credits;
	if (extract_json_value_for_key(body, "credits", &raw_credits)) {
		parse_backend_credit_summary(body, &profile->credits);
	}

	std::string raw_preferences;
	if (extract_json_value_for_key(body, "preferences", &raw_preferences)) {
		if (extract_json_value_for_key(raw_preferences, "ai_model", &raw_value)) {
			profile->preferences.ai_model = parse_json_string_value(raw_value);
		}
		if (extract_json_value_for_key(raw_preferences, "ai_image_model", &raw_value)) {
			profile->preferences.ai_image_model = parse_json_string_value(raw_value);
		}
	}
	return true;
}

bool parse_backend_texture_slot(const std::string &object_text, BackendTextureSlot *texture)
{
	if (texture == nullptr) {
		return false;
	}

	*texture = BackendTextureSlot{};
	std::string raw_value;
	if (extract_json_value_for_key(object_text, "slot", &raw_value)) {
		texture->slot = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "display_name", &raw_value)) {
		texture->display_name = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "active", &raw_value)) {
		texture->active = parse_json_bool_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "alpha", &raw_value)) {
		texture->alpha = parse_json_float_value(raw_value, 1.0f);
	}
	if (extract_json_value_for_key(object_text, "width", &raw_value)) {
		texture->width = parse_json_int_value(raw_value, 512);
	}
	if (extract_json_value_for_key(object_text, "height", &raw_value)) {
		texture->height = parse_json_int_value(raw_value, 512);
	}
	if (extract_json_value_for_key(object_text, "source", &raw_value)) {
		texture->source = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "prompt", &raw_value)) {
		texture->prompt = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "updated_at", &raw_value)) {
		texture->updated_at = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "image_url", &raw_value)) {
		texture->image_url = parse_json_string_value(raw_value);
	}
	if (texture->display_name.empty()) {
		texture->display_name = texture->slot;
	}
	return !texture->slot.empty();
}

bool parse_backend_texture_slots_from_body(const std::string &body,
                                           std::vector<BackendTextureSlot> *textures)
{
	if (textures == nullptr) {
		return false;
	}

	std::string raw_textures;
	if (!extract_json_value_for_key(body, "textures", &raw_textures)) {
		textures->clear();
		return true;
	}

	std::vector<BackendTextureSlot> parsed_textures;
	for (const std::string &object_text : extract_top_level_json_objects(raw_textures)) {
		BackendTextureSlot texture;
		if (parse_backend_texture_slot(object_text, &texture)) {
			parsed_textures.push_back(std::move(texture));
		}
	}

	*textures = std::move(parsed_textures);
	return true;
}

bool parse_backend_ai_thread_summary(const std::string &object_text,
                                     BackendAiThreadSummary *thread)
{
	if (thread == nullptr) {
		return false;
	}

	*thread = BackendAiThreadSummary{};
	std::string raw_value;
	if (extract_json_value_for_key(object_text, "id", &raw_value)) {
		thread->id = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "title", &raw_value)) {
		thread->title = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "mode", &raw_value)) {
		thread->mode = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "file_id", &raw_value)) {
		thread->file_id = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "file_title", &raw_value)) {
		thread->file_title = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "created_at", &raw_value)) {
		thread->created_at = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "updated_at", &raw_value)) {
		thread->updated_at = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "last_message_at", &raw_value)) {
		thread->last_message_at = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "last_message_preview", &raw_value)) {
		thread->last_message_preview = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "message_count", &raw_value)) {
		thread->message_count = parse_json_int_value(raw_value);
	}
	return !thread->id.empty();
}

bool parse_backend_ai_threads_from_body(const std::string &body,
                                        std::vector<BackendAiThreadSummary> *threads)
{
	if (threads == nullptr) {
		return false;
	}

	std::string raw_threads;
	if (!extract_json_value_for_key(body, "threads", &raw_threads)) {
		threads->clear();
		return true;
	}

	std::vector<BackendAiThreadSummary> parsed_threads;
	for (const std::string &object_text : extract_top_level_json_objects(raw_threads)) {
		BackendAiThreadSummary thread;
		if (parse_backend_ai_thread_summary(object_text, &thread)) {
			parsed_threads.push_back(std::move(thread));
		}
	}

	*threads = std::move(parsed_threads);
	return true;
}

bool parse_backend_ai_result(const std::string &object_text, BackendAiResult *result)
{
	if (result == nullptr) {
		return false;
	}

	*result = BackendAiResult{};
	result->raw_json = trim_copy(object_text);
	if (result->raw_json.empty() || result->raw_json.front() != '{') {
		result->raw_json.clear();
		return false;
	}

	std::string raw_value;
	bool has_any = false;
	const auto parse_string_field = [&](const char *key, std::string *target) {
		if (target == nullptr) {
			return;
		}
		if (extract_json_value_for_key(object_text, key, &raw_value)) {
			*target = parse_json_string_value(raw_value);
			has_any = true;
		}
	};
	const auto parse_list_field = [&](const char *key, std::vector<std::string> *target) {
		if (target == nullptr) {
			return;
		}
		if (extract_json_value_for_key(object_text, key, &raw_value)) {
			*target = parse_json_string_array(raw_value);
			has_any = true;
		}
	};

	parse_string_field("title", &result->title);
	parse_string_field("grammar", &result->grammar);
	parse_string_field("summary", &result->summary);
	parse_string_field("answer", &result->answer);
	parse_string_field("repair_summary", &result->repair_summary);
	parse_string_field("lesson", &result->lesson);
	parse_string_field("diagnosis", &result->diagnosis);
	parse_string_field("practice_prompt", &result->practice_prompt);
	parse_list_field("motifs", &result->motifs);
	parse_list_field("next_steps", &result->next_steps);
	parse_list_field("changes", &result->changes);
	parse_list_field("observations", &result->observations);
	parse_list_field("suggested_edits", &result->suggested_edits);
	parse_list_field("actions", &result->actions);
	parse_list_field("warnings", &result->warnings);

	result->loaded = has_any;
	if (!result->loaded) {
		result->raw_json.clear();
	}
	return result->loaded;
}

bool parse_backend_ai_message(const std::string &object_text, BackendAiMessage *message)
{
	if (message == nullptr) {
		return false;
	}

	*message = BackendAiMessage{};
	std::string raw_value;
	if (extract_json_value_for_key(object_text, "id", &raw_value)) {
		message->id = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "thread_id", &raw_value)) {
		message->thread_id = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "owner_uid", &raw_value)) {
		message->owner_uid = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "role", &raw_value)) {
		message->role = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "mode", &raw_value)) {
		message->mode = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "content", &raw_value)) {
		message->content = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "created_at", &raw_value)) {
		message->created_at = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "payload", &raw_value)) {
		message->payload_json = trim_copy(raw_value);
		parse_backend_ai_result(message->payload_json, &message->result);
	}
	return !message->id.empty();
}

bool parse_backend_ai_messages_from_raw(const std::string &raw_messages,
                                        std::vector<BackendAiMessage> *messages)
{
	if (messages == nullptr) {
		return false;
	}

	std::vector<BackendAiMessage> parsed_messages;
	for (const std::string &object_text : extract_top_level_json_objects(raw_messages)) {
		BackendAiMessage message;
		if (parse_backend_ai_message(object_text, &message)) {
			parsed_messages.push_back(std::move(message));
		}
	}
	*messages = std::move(parsed_messages);
	return true;
}

bool parse_backend_ai_messages_from_body(const std::string &body,
                                         std::vector<BackendAiMessage> *messages)
{
	if (messages == nullptr) {
		return false;
	}

	std::string raw_messages;
	if (!extract_json_value_for_key(body, "messages", &raw_messages)) {
		messages->clear();
		return true;
	}
	return parse_backend_ai_messages_from_raw(raw_messages, messages);
}

bool parse_backend_ai_usage_from_body(const std::string &body, BackendAiUsageSummary *usage)
{
	if (usage == nullptr) {
		return false;
	}

	*usage = BackendAiUsageSummary{};
	std::string raw_usage;
	if (!extract_json_value_for_key(body, "usage", &raw_usage)) {
		return false;
	}

	usage->loaded = true;
	std::string raw_value;
	if (extract_json_value_for_key(raw_usage, "id", &raw_value)) {
		usage->id = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(raw_usage, "status", &raw_value)) {
		usage->status = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(raw_usage, "estimated_credits", &raw_value)) {
		usage->estimated_credits = parse_json_int_value(raw_value);
	}
	if (extract_json_value_for_key(raw_usage, "final_credits", &raw_value)) {
		usage->final_credits = parse_json_int_value(raw_value);
	}
	if (extract_json_value_for_key(raw_usage, "prompt_tokens", &raw_value)) {
		usage->prompt_tokens = parse_json_int_value(raw_value);
	}
	if (extract_json_value_for_key(raw_usage, "completion_tokens", &raw_value)) {
		usage->completion_tokens = parse_json_int_value(raw_value);
	}
	if (extract_json_value_for_key(raw_usage, "total_tokens", &raw_value)) {
		usage->total_tokens = parse_json_int_value(raw_value);
	}
	return true;
}

bool parse_backend_ai_thread_from_body(const std::string &body,
                                       BackendAiThreadSummary *thread)
{
	if (thread == nullptr) {
		return false;
	}

	std::string raw_thread;
	if (!extract_json_value_for_key(body, "thread", &raw_thread)) {
		return false;
	}
	return parse_backend_ai_thread_summary(raw_thread, thread);
}

bool parse_backend_ai_result_from_body(const std::string &body, BackendAiResult *result)
{
	if (result == nullptr) {
		return false;
	}

	std::string raw_result;
	if (!extract_json_value_for_key(body, "result", &raw_result)) {
		return false;
	}
	return parse_backend_ai_result(raw_result, result);
}

bool parse_backend_file_summary(const std::string &object_text, BackendFileSummary *file)
{
	if (file == nullptr) {
		return false;
	}
	*file = BackendFileSummary{};
	std::string raw_value;
	if (extract_json_value_for_key(object_text, "id", &raw_value)) {
		file->id = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "title", &raw_value)) {
		file->title = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "updated_at", &raw_value)) {
		file->updated_at = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(object_text, "is_published", &raw_value)) {
		file->is_published = parse_json_bool_value(raw_value);
	}
	return !file->id.empty();
}

bool parse_backend_file_record_from_body(const std::string &body, BackendFileRecord *file)
{
	if (file == nullptr) {
		return false;
	}

	std::string raw_file;
	if (!extract_json_value_for_key(body, "file", &raw_file)) {
		return false;
	}

	BackendFileSummary summary;
	if (!parse_backend_file_summary(raw_file, &summary)) {
		return false;
	}

	*file = BackendFileRecord{};
	file->id = std::move(summary.id);
	file->title = std::move(summary.title);
	file->updated_at = std::move(summary.updated_at);
	file->is_published = summary.is_published;

	std::string raw_value;
	if (extract_json_value_for_key(raw_file, "content", &raw_value)) {
		file->content = parse_json_string_value(raw_value);
		file->has_content = true;
	}
	if (!file->has_content) {
		*file = BackendFileRecord{};
		return false;
	}
	return true;
}

bool backend_api_list_files_json_only(const FirebaseAuthConfig &config,
                                      FirebaseAuthSession *session,
                                      std::vector<BackendFileSummary> *files,
                                      std::string *error)
{
	if (files == nullptr) {
		if (error != nullptr) {
			*error = "File list output was null.";
		}
		return false;
	}

	HttpResponse response;
	if (!authorized_backend_request(config,
	                                session,
	                                "GET",
	                                "/api/file.php?action=list",
	                                "",
	                                &response,
	                                error)) {
		return false;
	}

	std::string raw_files;
	if (!extract_json_value_for_key(response.body, "files", &raw_files)) {
		if (error != nullptr) {
			*error = "Backend list response did not contain a files array.";
		}
		return false;
	}

	std::vector<BackendFileSummary> parsed_files;
	for (const std::string &object_text : extract_top_level_json_objects(raw_files)) {
		BackendFileSummary file;
		if (parse_backend_file_summary(object_text, &file)) {
			parsed_files.push_back(std::move(file));
		}
	}
	*files = std::move(parsed_files);
	return true;
}

bool backend_api_get_file_json_only(const FirebaseAuthConfig &config,
                                    FirebaseAuthSession *session,
                                    const std::string &file_id,
                                    BackendFileRecord *file,
                                    std::string *error)
{
	if (file == nullptr) {
		if (error != nullptr) {
			*error = "File output was null.";
		}
		return false;
	}

	const std::string normalized_file_id = trim_copy(file_id);
	if (normalized_file_id.empty()) {
		if (error != nullptr) {
			*error = "File id is empty.";
		}
		return false;
	}

	HttpResponse response;
	if (!authorized_backend_request(config,
	                                session,
	                                "GET",
	                                "/api/file.php?action=get&id=" + url_encode(normalized_file_id),
	                                "",
	                                &response,
	                                error)) {
		return false;
	}

	if (!parse_backend_file_record_from_body(response.body, file)) {
		if (error != nullptr) {
			*error = "Backend get-file response did not contain a readable file payload.";
		}
		return false;
	}
	return true;
}

bool parse_editor_bootstrap_file_payload(const std::string &body,
                                         BackendFileRecord *file,
                                         std::string *error)
{
	std::string raw_payload;
	if (!extract_script_json_assignment(body, "const initialFilePayload =", &raw_payload)) {
		if (error != nullptr) {
			*error = "The server file-open API failed and the editor page did not expose a recovery payload.";
		}
		return false;
	}

	if (trim_copy(raw_payload) == "null") {
		if (error != nullptr) {
			*error = "The server file-open API failed and the editor page did not preload the requested file.";
		}
		return false;
	}

	if (parse_backend_file_record_from_body(raw_payload, file)) {
		return true;
	}

	if (error != nullptr) {
		std::string raw_error;
		std::string raw_detail;
		std::string message = "The editor page returned an unreadable file payload.";
		if (extract_json_value_for_key(raw_payload, "error", &raw_error)) {
			const std::string parsed = parse_json_string_value(raw_error);
			if (!parsed.empty()) {
				message = parsed;
			}
		}
		if (extract_json_value_for_key(raw_payload, "detail", &raw_detail)) {
			const std::string parsed = parse_json_string_value(raw_detail);
			if (!parsed.empty()) {
				message += " " + parsed;
			}
		}
		*error = message;
	}
	return false;
}

bool fetch_editor_page_file_fallback(const FirebaseAuthConfig &config,
                                     FirebaseAuthSession *session,
                                     const std::string &file_id,
                                     BackendFileRecord *file,
                                     std::string *error)
{
	BackendUserProfile ignored_profile;
	std::string login_error;
	if (!backend_api_login(config, session, &ignored_profile, &login_error)) {
		if (error != nullptr) {
			*error = login_error;
		}
		return false;
	}

	HttpResponse page_response;
	const std::vector<std::string> headers{
		"Accept: text/html,application/xhtml+xml",
	};
	const std::string url =
		config.backend_base_url + "/editor.php?file=" + url_encode(trim_copy(file_id));
	if (!perform_request("GET", url, "", headers, &page_response)) {
		if (error != nullptr) {
			*error = response_error_message(page_response);
		}
		return false;
	}
	if (page_response.status_code < 200 || page_response.status_code >= 300) {
		if (error != nullptr) {
			*error = response_error_message(page_response);
		}
		return false;
	}

	if (parse_editor_bootstrap_file_payload(page_response.body, file, error)) {
		return true;
	}

	if (error != nullptr && error->empty()) {
		*error = "The editor page did not provide a compatible file payload.";
	}
	return false;
}

std::string build_save_payload(const std::string &file_id,
                               const std::string &title,
                               const std::string &content)
{
	std::ostringstream payload;
	payload << "{\n";
	if (!trim_copy(file_id).empty()) {
		payload << "  \"file_id\": \"" << json_escape(trim_copy(file_id)) << "\",\n";
	}
	payload << "  \"title\": \"" << json_escape(title) << "\",\n"
	        << "  \"content\": \"" << json_escape(content) << "\"\n"
	        << "}";
	return payload.str();
}

std::string build_texture_payload(const std::string &slot,
                                  const std::string &display_name,
                                  float alpha,
                                  const std::string &prompt = "")
{
	std::ostringstream payload;
	payload << "{\n"
	        << "  \"slot\": \"" << json_escape(trim_copy(slot)) << "\",\n"
	        << "  \"display_name\": \"" << json_escape(trim_copy(display_name)) << "\",\n"
	        << "  \"alpha\": " << std::max(0.0f, std::min(1.0f, alpha));
	if (!prompt.empty()) {
		payload << ",\n  \"prompt\": \"" << json_escape(trim_copy(prompt)) << "\"\n";
	} else {
		payload << "\n";
	}
	payload << "}";
	return payload.str();
}

std::string build_ai_payload(const BackendAiGenerateRequest &request)
{
	std::ostringstream payload;
	payload << "{\n"
	        << "  \"mode\": \"" << json_escape(trim_copy(request.mode)) << "\"";

	const auto append_string_field = [&](const char *key, const std::string &value) {
		if (trim_copy(value).empty()) {
			return;
		}
		payload << ",\n  \"" << key << "\": \"" << json_escape(value) << "\"";
	};

	append_string_field("prompt", trim_copy(request.prompt));
	append_string_field("question", trim_copy(request.question));
	append_string_field("grammar", request.grammar);
	append_string_field("selection", request.selection);
	append_string_field("parserError", request.parser_error);
	append_string_field("thread_id", trim_copy(request.thread_id));
	append_string_field("file_id", trim_copy(request.file_id));
	append_string_field("title", trim_copy(request.title));
	if (!request.history.empty()) {
		payload << ",\n  \"history\": [";
		for (std::size_t index = 0; index < request.history.size(); ++index) {
			if (index > 0) {
				payload << ", ";
			}
			payload << "\"" << json_escape(request.history[index]) << "\"";
		}
		payload << "]";
	}

	payload << "\n}";
	return payload.str();
}

}  // namespace

bool backend_api_login(const FirebaseAuthConfig &config,
                       FirebaseAuthSession *session,
                       BackendUserProfile *profile,
                       std::string *error)
{
	HttpResponse response;
	if (!authorized_backend_request(config,
	                                session,
	                                "POST",
	                                "/api/session.php?action=login",
	                                "{}",
	                                &response,
	                                error)) {
		return false;
	}
	if (profile != nullptr) {
		parse_backend_user_profile(response.body, profile);
	}
	return true;
}

bool backend_api_list_files(const FirebaseAuthConfig &config,
                            FirebaseAuthSession *session,
                            std::vector<BackendFileSummary> *files,
                            std::string *error)
{
	if (files == nullptr) {
		if (error != nullptr) {
			*error = "File list output was null.";
		}
		return false;
	}
	if (load_cached_file_list(files)) {
		return true;
	}
	if (!backend_api_list_files_json_only(config, session, files, error)) {
		return false;
	}
	store_cached_file_list(*files);
	return true;
}

bool backend_api_get_file(const FirebaseAuthConfig &config,
                          FirebaseAuthSession *session,
                          const std::string &file_id,
                          BackendFileRecord *file,
                          std::string *error)
{
	if (file == nullptr) {
		if (error != nullptr) {
			*error = "File output was null.";
		}
		return false;
	}
	if (trim_copy(file_id).empty()) {
		if (error != nullptr) {
			*error = "File id is empty.";
		}
		return false;
	}
	if (load_cached_file(file_id, file)) {
		return true;
	}
	if (!backend_api_get_file_json_only(config, session, file_id, file, error)) {
		return false;
	}
	store_cached_file(*file);
	return true;
}

bool backend_api_get_public_file(const FirebaseAuthConfig &config,
                                 FirebaseAuthSession *session,
                                 const std::string &file_id,
                                 BackendFileRecord *file,
                                 std::string *error)
{
	if (file == nullptr) {
		if (error != nullptr) {
			*error = "File output was null.";
		}
		return false;
	}
	if (trim_copy(file_id).empty()) {
		if (error != nullptr) {
			*error = "File id is empty.";
		}
		return false;
	}

	HttpResponse response;
	if (!authorized_backend_request(config,
	                                session,
	                                "GET",
	                                "/api/file.php?action=get_public&id=" + url_encode(trim_copy(file_id)),
	                                "",
	                                &response,
	                                error)) {
		return false;
	}
	if (!parse_backend_file_record_from_body(response.body, file)) {
		if (error != nullptr) {
			*error = "Backend returned an unreadable public file payload.";
		}
		return false;
	}
	store_cached_file(*file);
	return true;
}

bool backend_api_save_file(const FirebaseAuthConfig &config,
                           FirebaseAuthSession *session,
                           const std::string &file_id,
                           const std::string &title,
                           const std::string &content,
                           BackendFileRecord *file,
                           std::string *error)
{
	if (trim_copy(title).empty()) {
		if (error != nullptr) {
			*error = "Cloud title is empty.";
		}
		return false;
	}
	if (content.size() > kBackendGrammarMaxBytes) {
		if (error != nullptr) {
			*error = "Grammar exceeds the 30,000 byte backend limit.";
		}
		return false;
	}

	HttpResponse response;
	if (!authorized_backend_request(config,
	                                session,
	                                "POST",
	                                "/api/file.php?action=save",
	                                build_save_payload(file_id, trim_copy(title), content),
	                                &response,
	                                error)) {
		return false;
	}

	if (file != nullptr) {
		*file = BackendFileRecord{};
		parse_backend_file_record_from_body(response.body, file);
		file->id = file->id.empty() ? trim_copy(file_id) : file->id;
		file->title = file->title.empty() ? trim_copy(title) : file->title;
		file->content = content;
		file->has_content = true;
		file->is_published = false;
		store_cached_file(*file);
	}
	invalidate_cached_file_list();
	return true;
}

bool backend_api_publish_file(const FirebaseAuthConfig &config,
                              FirebaseAuthSession *session,
                              const std::string &file_id,
                              const std::string &title,
                              const std::string &content,
                              BackendFileRecord *file,
                              std::string *error)
{
	if (trim_copy(file_id).empty()) {
		if (error != nullptr) {
			*error = "Save the grammar to the cloud before publishing it.";
		}
		return false;
	}
	if (trim_copy(title).empty()) {
		if (error != nullptr) {
			*error = "Cloud title is empty.";
		}
		return false;
	}
	if (content.size() > kBackendGrammarMaxBytes) {
		if (error != nullptr) {
			*error = "Grammar exceeds the 30,000 byte backend limit.";
		}
		return false;
	}

	HttpResponse response;
	if (!authorized_backend_request(config,
	                                session,
	                                "POST",
	                                "/api/file.php?action=publish",
	                                build_save_payload(file_id, trim_copy(title), content),
	                                &response,
	                                error)) {
		return false;
	}

	if (file != nullptr) {
		*file = BackendFileRecord{};
		parse_backend_file_record_from_body(response.body, file);
		file->id = file->id.empty() ? trim_copy(file_id) : file->id;
		file->title = file->title.empty() ? trim_copy(title) : file->title;
		file->content = content;
		file->has_content = true;
		file->is_published = true;
		store_cached_file(*file);
	}
	invalidate_cached_file_list();
	return true;
}

bool backend_api_unpublish_file(const FirebaseAuthConfig &config,
                                FirebaseAuthSession *session,
                                const std::string &file_id,
                                std::string *error)
{
	if (trim_copy(file_id).empty()) {
		if (error != nullptr) {
			*error = "Save the grammar to the cloud before depublishing it.";
		}
		return false;
	}

	std::ostringstream payload;
	payload << "{\n"
	        << "  \"file_id\": \"" << json_escape(trim_copy(file_id)) << "\"\n"
	        << "}";

	HttpResponse response;
	const bool ok = authorized_backend_request(config,
	                                           session,
	                                           "POST",
	                                           "/api/file.php?action=unpublish",
	                                           payload.str(),
	                                           &response,
	                                           error);
	if (ok) {
		erase_cached_file(file_id);
		invalidate_cached_file_list();
	}
	return ok;
}

bool backend_api_list_textures(const FirebaseAuthConfig &config,
                               FirebaseAuthSession *session,
                               std::vector<BackendTextureSlot> *textures,
                               std::string *error)
{
	if (textures == nullptr) {
		if (error != nullptr) {
			*error = "Texture list output was null.";
		}
		return false;
	}

	HttpResponse response;
	if (!authorized_backend_request(config,
	                                session,
	                                "GET",
	                                "/api/textures.php?action=list",
	                                "",
	                                &response,
	                                error)) {
		return false;
	}

	return parse_backend_texture_slots_from_body(response.body, textures);
}

bool backend_api_fetch_texture_image(const FirebaseAuthConfig &config,
                                     FirebaseAuthSession *session,
                                     const std::string &slot,
                                     std::string *png_bytes,
                                     std::string *error)
{
	if (png_bytes == nullptr) {
		if (error != nullptr) {
			*error = "Texture image output was null.";
		}
		return false;
	}
	if (trim_copy(slot).empty()) {
		if (error != nullptr) {
			*error = "Texture slot is empty.";
		}
		return false;
	}

	HttpResponse response;
	if (!authorized_backend_request(config,
	                                session,
	                                "GET",
	                                "/api/textures.php?action=image&slot=" + trim_copy(slot),
	                                "",
	                                &response,
	                                error)) {
		return false;
	}

	*png_bytes = std::move(response.body);
	return true;
}

bool backend_api_update_texture(const FirebaseAuthConfig &config,
                                FirebaseAuthSession *session,
                                const std::string &slot,
                                const std::string &display_name,
                                float alpha,
                                std::vector<BackendTextureSlot> *textures,
                                std::string *error)
{
	if (trim_copy(slot).empty()) {
		if (error != nullptr) {
			*error = "Texture slot is empty.";
		}
		return false;
	}
	if (textures == nullptr) {
		if (error != nullptr) {
			*error = "Texture list output was null.";
		}
		return false;
	}

	HttpResponse response;
	if (!authorized_backend_request(config,
	                                session,
	                                "POST",
	                                "/api/textures.php?action=update",
	                                build_texture_payload(slot, display_name, alpha),
	                                &response,
	                                error)) {
		return false;
	}

	return parse_backend_texture_slots_from_body(response.body, textures);
}

bool backend_api_delete_texture(const FirebaseAuthConfig &config,
                                FirebaseAuthSession *session,
                                const std::string &slot,
                                std::vector<BackendTextureSlot> *textures,
                                std::string *error)
{
	if (trim_copy(slot).empty()) {
		if (error != nullptr) {
			*error = "Texture slot is empty.";
		}
		return false;
	}
	if (textures == nullptr) {
		if (error != nullptr) {
			*error = "Texture list output was null.";
		}
		return false;
	}

	std::ostringstream payload;
	payload << "{\n"
	        << "  \"slot\": \"" << json_escape(trim_copy(slot)) << "\"\n"
	        << "}";

	HttpResponse response;
	if (!authorized_backend_request(config,
	                                session,
	                                "POST",
	                                "/api/textures.php?action=delete",
	                                payload.str(),
	                                &response,
	                                error)) {
		return false;
	}

	return parse_backend_texture_slots_from_body(response.body, textures);
}

bool backend_api_upload_texture(const FirebaseAuthConfig &config,
                                FirebaseAuthSession *session,
                                const std::string &slot,
                                const std::string &display_name,
                                float alpha,
                                const std::string &local_path,
                                std::vector<BackendTextureSlot> *textures,
                                std::string *error)
{
	if (trim_copy(slot).empty()) {
		if (error != nullptr) {
			*error = "Texture slot is empty.";
		}
		return false;
	}
	if (trim_copy(local_path).empty()) {
		if (error != nullptr) {
			*error = "Texture upload path is empty.";
		}
		return false;
	}
	if (!std::filesystem::exists(local_path)) {
		if (error != nullptr) {
			*error = "Texture upload file was not found.";
		}
		return false;
	}
	if (textures == nullptr) {
		if (error != nullptr) {
			*error = "Texture list output was null.";
		}
		return false;
	}

	const std::vector<MultipartField> fields{
		{"slot", trim_copy(slot)},
		{"display_name", trim_copy(display_name)},
		{"alpha", std::to_string(std::max(0.0f, std::min(1.0f, alpha)))},
	};

	HttpResponse response;
	if (!authorized_backend_multipart_request(config,
	                                          session,
	                                          "/api/textures.php?action=upload",
	                                          fields,
	                                          "texture",
	                                          local_path,
	                                          &response,
	                                          error)) {
		return false;
	}

	return parse_backend_texture_slots_from_body(response.body, textures);
}

bool backend_api_generate_texture(const FirebaseAuthConfig &config,
                                  FirebaseAuthSession *session,
                                  const std::string &slot,
                                  const std::string &display_name,
                                  float alpha,
                                  const std::string &prompt,
                                  std::vector<BackendTextureSlot> *textures,
                                  BackendCreditSummary *credits,
                                  std::string *error)
{
	if (trim_copy(slot).empty()) {
		if (error != nullptr) {
			*error = "Texture slot is empty.";
		}
		return false;
	}
	if (trim_copy(prompt).empty()) {
		if (error != nullptr) {
			*error = "Describe the texture you want to generate.";
		}
		return false;
	}
	if (textures == nullptr) {
		if (error != nullptr) {
			*error = "Texture list output was null.";
		}
		return false;
	}

	HttpResponse response;
	if (!authorized_backend_request(config,
	                                session,
	                                "POST",
	                                "/api/textures.php?action=generate",
	                                build_texture_payload(slot, display_name, alpha, prompt),
	                                &response,
	                                error)) {
		return false;
	}

	if (!parse_backend_texture_slots_from_body(response.body, textures)) {
		return false;
	}
	if (credits != nullptr) {
		parse_backend_credit_summary(response.body, credits);
	}
	return true;
}

bool backend_api_list_ai_threads(const FirebaseAuthConfig &config,
                                 FirebaseAuthSession *session,
                                 std::vector<BackendAiThreadSummary> *threads,
                                 std::string *error)
{
	if (threads == nullptr) {
		if (error != nullptr) {
			*error = "AI thread list output was null.";
		}
		return false;
	}

	HttpResponse response;
	if (!authorized_backend_request(config,
	                                session,
	                                "GET",
	                                "/api/ai.php?action=list_threads",
	                                "",
	                                &response,
	                                error)) {
		return false;
	}

	return parse_backend_ai_threads_from_body(response.body, threads);
}

bool backend_api_get_ai_thread(const FirebaseAuthConfig &config,
                               FirebaseAuthSession *session,
                               const std::string &thread_id,
                               BackendAiThreadSummary *thread,
                               std::vector<BackendAiMessage> *messages,
                               std::string *error)
{
	if (trim_copy(thread_id).empty()) {
		if (error != nullptr) {
			*error = "AI thread id is empty.";
		}
		return false;
	}
	if (thread == nullptr || messages == nullptr) {
		if (error != nullptr) {
			*error = "AI thread outputs were null.";
		}
		return false;
	}

	HttpResponse response;
	if (!authorized_backend_request(config,
	                                session,
	                                "GET",
	                                "/api/ai.php?action=get_thread&thread_id=" + trim_copy(thread_id),
	                                "",
	                                &response,
	                                error)) {
		return false;
	}

	if (!parse_backend_ai_thread_from_body(response.body, thread)) {
		if (error != nullptr) {
			*error = "Backend returned an unreadable AI thread payload.";
		}
		return false;
	}
	if (!parse_backend_ai_messages_from_body(response.body, messages)) {
		if (error != nullptr) {
			*error = "Backend returned unreadable AI messages.";
		}
		return false;
	}
	return true;
}

bool backend_api_generate_ai(const FirebaseAuthConfig &config,
                             FirebaseAuthSession *session,
                             const BackendAiGenerateRequest &request,
                             BackendAiGenerateResponse *response,
                             std::string *error)
{
	if (response == nullptr) {
		if (error != nullptr) {
			*error = "AI response output was null.";
		}
		return false;
	}

	*response = BackendAiGenerateResponse{};
	const std::string mode = trim_copy(request.mode);
	if (mode.empty()) {
		if (error != nullptr) {
			*error = "AI mode is empty.";
		}
		return false;
	}
	if (mode == "draft_grammar" && trim_copy(request.prompt).empty()) {
		if (error != nullptr) {
			*error = "Draft grammar mode requires prompt text.";
		}
		return false;
	}
	if ((mode == "repair_grammar" || mode == "explain_grammar" || mode == "tutor_next_step") &&
	    trim_copy(request.prompt).empty() &&
	    trim_copy(request.question).empty() &&
	    trim_copy(request.grammar).empty()) {
		if (error != nullptr) {
			*error = "This AI mode requires grammar or prompt context.";
		}
		return false;
	}

	HttpResponse raw_response;
	const bool ok = authorized_backend_request(config,
	                                           session,
	                                           "POST",
	                                           "/api/ai.php?action=generate",
	                                           build_ai_payload(request),
	                                           &raw_response,
	                                           error);
	parse_backend_credit_summary(raw_response.body, &response->credits);
	if (!ok) {
		return false;
	}

	std::string raw_value;
	if (extract_json_value_for_key(raw_response.body, "mode", &raw_value)) {
		response->mode = parse_json_string_value(raw_value);
	}
	if (extract_json_value_for_key(raw_response.body, "model", &raw_value)) {
		response->model = parse_json_string_value(raw_value);
	}
	parse_backend_ai_thread_from_body(raw_response.body, &response->thread);
	parse_backend_ai_usage_from_body(raw_response.body, &response->usage);
	parse_backend_ai_messages_from_body(raw_response.body, &response->messages);
	parse_backend_ai_result_from_body(raw_response.body, &response->result);
	return true;
}

BackendAsyncToken backend_api_list_files_async(const FirebaseAuthConfig &config,
                                               FirebaseAuthSession session,
                                               BackendListFilesCallback callback)
{
	BackendAsyncToken token{std::make_shared<std::atomic<bool>>(false)};
	if (token.cancelled == nullptr) {
		return token;
	}

	std::vector<BackendFileSummary> cached_files;
	if (load_cached_file_list(&cached_files)) {
		BackendAsyncResult<std::vector<BackendFileSummary>> result;
		result.markSucceeded();
		result.value = std::move(cached_files);
		if (callback != nullptr) {
			callback(result);
		}
		return token;
	}

	bool should_start_worker = false;
	{
		std::lock_guard<std::mutex> lock(g_inflight_mutex);
		g_pending_list_callbacks.emplace_back(token.cancelled, std::move(callback));
		if (!g_list_request_running) {
			g_list_request_running = true;
			should_start_worker = true;
		}
	}

	if (!should_start_worker) {
		return token;
	}

	std::thread([config, session]() mutable {
		BackendAsyncResult<std::vector<BackendFileSummary>> result;

		std::vector<BackendFileSummary> files;
		std::string error;
		if (backend_api_list_files_json_only(config, &session, &files, &error)) {
			store_cached_file_list(files);
			result.markSucceeded();
			result.value = std::move(files);
		} else {
			result.markFailed(error.empty() ? "Unable to list cloud files." : error);
		}

		std::vector<std::pair<std::shared_ptr<std::atomic<bool>>, BackendListFilesCallback>>
			callbacks_to_run;
		{
			std::lock_guard<std::mutex> lock(g_inflight_mutex);
			callbacks_to_run.swap(g_pending_list_callbacks);
			g_list_request_running = false;
		}

		for (const auto &pending : callbacks_to_run) {
			const std::shared_ptr<std::atomic<bool>> &cancelled = pending.first;
			const BackendListFilesCallback &pending_callback = pending.second;
			if (pending_callback == nullptr) {
				continue;
			}
			if (cancelled != nullptr && cancelled->load()) {
				continue;
			}
			pending_callback(result);
		}
	}).detach();

	return token;
}

BackendAsyncToken backend_api_get_file_async(const FirebaseAuthConfig &config,
                                             FirebaseAuthSession session,
                                             std::string file_id,
                                             BackendGetFileCallback callback)
{
	BackendAsyncToken token{std::make_shared<std::atomic<bool>>(false)};
	if (token.cancelled == nullptr) {
		return token;
	}

	const std::string normalized_file_id = trim_copy(file_id);
	if (normalized_file_id.empty()) {
		BackendAsyncResult<BackendFileRecord> result;
		result.markFailed("File id is empty.");
		if (callback != nullptr) {
			callback(result);
		}
		return token;
	}

	BackendFileRecord cached_file;
	if (load_cached_file(normalized_file_id, &cached_file)) {
		BackendAsyncResult<BackendFileRecord> result;
		result.markSucceeded();
		result.value = std::move(cached_file);
		if (callback != nullptr) {
			callback(result);
		}
		return token;
	}

	bool should_start_worker = false;
	{
		std::lock_guard<std::mutex> lock(g_inflight_mutex);
		auto &pending_callbacks = g_pending_file_callbacks[normalized_file_id];
		pending_callbacks.emplace_back(token.cancelled, std::move(callback));
		should_start_worker = pending_callbacks.size() == 1;
	}

	if (!should_start_worker) {
		return token;
	}

	std::thread([config, session, normalized_file_id]() mutable {
		BackendAsyncResult<BackendFileRecord> result;

		BackendFileRecord file;
		std::string error;
		if (backend_api_get_file_json_only(config, &session, normalized_file_id, &file, &error)) {
			store_cached_file(file);
			result.markSucceeded();
			result.value = std::move(file);
		} else {
			result.markFailed(error.empty() ? "Unable to open cloud file." : error);
		}

		std::vector<std::pair<std::shared_ptr<std::atomic<bool>>, BackendGetFileCallback>>
			callbacks_to_run;
		{
			std::lock_guard<std::mutex> lock(g_inflight_mutex);
			auto it = g_pending_file_callbacks.find(normalized_file_id);
			if (it != g_pending_file_callbacks.end()) {
				callbacks_to_run.swap(it->second);
				g_pending_file_callbacks.erase(it);
			}
		}

		for (const auto &pending : callbacks_to_run) {
			const std::shared_ptr<std::atomic<bool>> &cancelled = pending.first;
			const BackendGetFileCallback &pending_callback = pending.second;
			if (pending_callback == nullptr) {
				continue;
			}
			if (cancelled != nullptr && cancelled->load()) {
				continue;
			}
			pending_callback(result);
		}
	}).detach();

	return token;
}

void backend_api_cancel_async(const BackendAsyncToken &token)
{
	if (token.cancelled != nullptr) {
		token.cancelled->store(true);
	}
}

bool backend_api_try_get_cached_file_list(std::vector<BackendFileSummary> *files)
{
	return load_cached_file_list(files);
}

bool backend_api_try_get_cached_file(const std::string &file_id, BackendFileRecord *file)
{
	return load_cached_file(file_id, file);
}

void backend_api_clear_cache()
{
	clear_backend_cache_internal();
}
