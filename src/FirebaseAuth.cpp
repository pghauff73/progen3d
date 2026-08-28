#include "FirebaseAuth.h"

#include "AppPaths.h"

#include <array>
#include <cstdlib>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <curl/curl.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

constexpr const char *kFirebaseAuthConfigFilename = "firebase_auth_config.json";
constexpr const char *kFirebaseAuthSessionFilename = "firebase_auth_session.json";
constexpr const char *kPreferredBackendBaseUrl = "https://www.xoiam.com";
constexpr std::array<const char *, 2> kGoogleServicesCandidates = {
	"google-services-desktop.json",
	"google-services.json"
};

std::once_flag g_curl_init_once;
bool g_curl_initialized = false;
std::string g_curl_init_error;
std::once_flag g_curl_tls_once;
std::string g_curl_ca_file;
std::string g_curl_ca_path;

struct HttpResponse {
	long status_code = 0;
	std::string body;
	std::string error;
};

std::filesystem::path firebase_auth_config_state_path()
{
	return progen3d_state_path(kFirebaseAuthConfigFilename);
}

std::filesystem::path firebase_auth_session_state_path()
{
	return progen3d_state_path(kFirebaseAuthSessionFilename);
}

std::filesystem::path firebase_auth_bundled_config_path()
{
	return progen3d_resource_path(std::filesystem::path("assets") / "config" / kFirebaseAuthConfigFilename);
}

std::filesystem::path firebase_auth_bundled_google_services_path(const char *filename)
{
	if (filename == nullptr || filename[0] == '\0') {
		return progen3d_resource_path("");
	}
	return progen3d_resource_path(std::filesystem::path("assets") / "config" / filename);
}

bool run_auth_exchange(const std::string &url,
                       const std::string &payload,
                       const char *content_type,
                       FirebaseAuthSession *session,
                       std::string *error);

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

std::string normalize_backend_base_url(std::string url)
{
	url = trim_copy(url);
	if (url.empty()) {
		return kPreferredBackendBaseUrl;
	}
	replace_all_inplace(&url, "progen3d.com", "xoiam.com");
	if (url.rfind("http://", 0) == 0) {
		url = "https://" + url.substr(std::string("http://").size());
	} else if (url.find("://") == std::string::npos) {
		url = "https://" + url;
	}
	while (url.size() > 1 && url.back() == '/') {
		url.pop_back();
	}
	return url;
}

std::string getenv_string(const char *name)
{
	if (name == nullptr || name[0] == '\0') {
		return "";
	}
	const char *value = std::getenv(name);
	if (value == nullptr || value[0] == '\0') {
		return "";
	}
	return trim_copy(value);
}

std::string join_nonempty(const std::vector<std::string> &parts, const std::string &separator)
{
	std::string output;
	for (const std::string &part : parts) {
		if (part.empty()) {
			continue;
		}
		if (!output.empty()) {
			output += separator;
		}
		output += part;
	}
	return output;
}

bool is_readable_regular_file(const std::filesystem::path &path)
{
	std::error_code error;
	return !path.empty() && std::filesystem::is_regular_file(path, error);
}

bool is_readable_directory(const std::filesystem::path &path)
{
	std::error_code error;
	return !path.empty() && std::filesystem::is_directory(path, error);
}

void initialize_curl_tls_paths()
{
	const std::array<const char *, 3> ca_file_env_candidates = {
		"PROGEN3D_CA_BUNDLE",
		"CURL_CA_BUNDLE",
		"SSL_CERT_FILE"
	};
	for (const char *name : ca_file_env_candidates) {
		const std::string value = getenv_string(name);
		if (value.empty()) {
			continue;
		}
		if (is_readable_regular_file(std::filesystem::path(value))) {
			g_curl_ca_file = value;
			break;
		}
	}

	const std::array<const char *, 1> ca_dir_env_candidates = {
		"SSL_CERT_DIR"
	};
	for (const char *name : ca_dir_env_candidates) {
		const std::string value = getenv_string(name);
		if (value.empty()) {
			continue;
		}
		if (is_readable_directory(std::filesystem::path(value))) {
			g_curl_ca_path = value;
			break;
		}
	}

	const std::array<std::filesystem::path, 8> ca_file_candidates = {
		progen3d_resource_path("cacert.pem"),
		progen3d_resource_path(std::filesystem::path("certs") / "cacert.pem"),
		progen3d_resource_path(std::filesystem::path("tls") / "cacert.pem"),
		"/etc/ssl/certs/ca-certificates.crt",
		"/etc/pki/tls/certs/ca-bundle.crt",
		"/etc/ssl/ca-bundle.pem",
		"/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",
		"/etc/ssl/cert.pem"
	};
	if (g_curl_ca_file.empty()) {
		for (const std::filesystem::path &path : ca_file_candidates) {
			if (!is_readable_regular_file(path)) {
				continue;
			}
			g_curl_ca_file = path.string();
			break;
		}
	}

	const std::array<std::filesystem::path, 4> ca_dir_candidates = {
		"/etc/ssl/certs",
		"/etc/pki/tls/certs",
		"/etc/openssl/certs",
		"/system/etc/security/cacerts"
	};
	if (g_curl_ca_path.empty()) {
		for (const std::filesystem::path &path : ca_dir_candidates) {
			if (!is_readable_directory(path)) {
				continue;
			}
			g_curl_ca_path = path.string();
			break;
		}
	}
}

std::string build_ssl_diagnostics_message(CURL *curl,
                                          CURLcode result,
                                          const char *error_buffer,
                                          const std::string &url)
{
	std::vector<std::string> parts;
	parts.push_back("curl=" + std::to_string(static_cast<int>(result)));
	parts.push_back(std::string("message=") + curl_easy_strerror(result));

	if (error_buffer != nullptr && error_buffer[0] != '\0') {
		parts.push_back(std::string("detail=") + error_buffer);
	}

	char *effective_url = nullptr;
	if (curl != nullptr &&
	    curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url) == CURLE_OK &&
	    effective_url != nullptr && effective_url[0] != '\0') {
		parts.push_back(std::string("effective_url=") + effective_url);
	} else if (!url.empty()) {
		parts.push_back(std::string("url=") + url);
	}

	long ssl_verify_result = 0;
	if (curl != nullptr &&
	    curl_easy_getinfo(curl, CURLINFO_SSL_VERIFYRESULT, &ssl_verify_result) == CURLE_OK) {
		parts.push_back("ssl_verify_result=" + std::to_string(ssl_verify_result));
	}

	const std::string cainfo = getenv_string("PROGEN3D_CURL_CAINFO");
	const std::string capath = getenv_string("PROGEN3D_CURL_CAPATH");
	if (!cainfo.empty()) {
		parts.push_back("cainfo=" + cainfo);
	}
	if (!capath.empty()) {
		parts.push_back("capath=" + capath);
	}

	return "TLS request failed: " + join_nonempty(parts, "; ");
}

bool read_text_file(const std::filesystem::path &path, std::string *out_text)
{
	if (out_text == nullptr) {
		return false;
	}
	std::ifstream input(path);
	if (!input.good()) {
		return false;
	}
	std::ostringstream buffer;
	buffer << input.rdbuf();
	*out_text = buffer.str();
	return true;
}

bool write_text_file(const std::filesystem::path &path,
                     const std::string &text,
                     std::string *error)
{
	if (path.has_parent_path()) {
		std::error_code create_error;
		std::filesystem::create_directories(path.parent_path(), create_error);
		if (create_error) {
			if (error != nullptr) {
				*error = "Unable to create parent directory for " + path.string();
			}
			return false;
		}
	}
	std::ofstream output(path, std::ios::out | std::ios::trunc);
	if (!output.good()) {
		if (error != nullptr) {
			*error = "Unable to write " + path.string();
		}
		return false;
	}
	output << text;
	if (!output.good()) {
		if (error != nullptr) {
			*error = "Failed while writing " + path.string();
		}
		return false;
	}
#if !defined(_WIN32)
	std::error_code permission_error;
	std::filesystem::permissions(
		path,
		std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
		std::filesystem::perm_options::replace,
		permission_error);
	if (permission_error) {
		if (error != nullptr) {
			*error = "Unable to secure permissions on " + path.string();
		}
		return false;
	}
#endif
	return true;
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

std::optional<std::string> extract_json_string_for_key(const std::string &text,
                                                       const std::string &key)
{
	const std::string needle = "\"" + key + "\"";
	std::size_t position = 0;
	while ((position = text.find(needle, position)) != std::string::npos) {
		std::size_t colon = text.find(':', position + needle.size());
		if (colon == std::string::npos) {
			return std::nullopt;
		}
		std::size_t value_start = colon + 1;
		while (value_start < text.size() &&
		       std::isspace(static_cast<unsigned char>(text[value_start])) != 0) {
			++value_start;
		}
		if (value_start >= text.size() || text[value_start] != '"') {
			position += needle.size();
			continue;
		}
		++value_start;
		std::string result;
		bool escaped = false;
		for (std::size_t index = value_start; index < text.size(); ++index) {
			const char ch = text[index];
			if (escaped) {
				switch (ch) {
				case '\\':
				case '"':
				case '/':
					result.push_back(ch);
					break;
				case 'n':
					result.push_back('\n');
					break;
				case 'r':
					result.push_back('\r');
					break;
				case 't':
					result.push_back('\t');
					break;
				default:
					result.push_back(ch);
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
				return result;
			}
			result.push_back(ch);
		}
		return std::nullopt;
	}
	return std::nullopt;
}

std::string extract_first_json_string(const std::string &text,
                                      std::initializer_list<const char *> keys)
{
	for (const char *key : keys) {
		if (key == nullptr) {
			continue;
		}
		const std::optional<std::string> value = extract_json_string_for_key(text, key);
		if (value.has_value()) {
			return *value;
		}
	}
	return "";
}

void assign_if_empty(std::string *target, const std::string &value)
{
	if (target == nullptr || !target->empty() || value.empty()) {
		return;
	}
	*target = value;
}

void merge_firebase_auth_config_from_json(FirebaseAuthConfig *config,
                                          const std::string &contents,
                                          bool allow_generic_google_oauth_keys)
{
	if (config == nullptr) {
		return;
	}

	assign_if_empty(&config->api_key,
	                extract_first_json_string(contents, {"apiKey", "api_key", "current_key"}));
	assign_if_empty(&config->project_id,
	                extract_first_json_string(contents, {"projectId", "project_id"}));
	assign_if_empty(&config->auth_domain,
	                extract_first_json_string(contents, {"authDomain", "auth_domain"}));
	assign_if_empty(&config->storage_bucket,
	                extract_first_json_string(contents, {"storageBucket", "storage_bucket"}));
	assign_if_empty(&config->google_client_id,
	                allow_generic_google_oauth_keys
	                    ? extract_first_json_string(contents,
	                                                {"googleClientId",
	                                                 "google_client_id",
	                                                 "client_id"})
	                    : extract_first_json_string(contents,
	                                                {"googleClientId", "google_client_id"}));
	assign_if_empty(&config->google_client_secret,
	                allow_generic_google_oauth_keys
	                    ? extract_first_json_string(contents,
	                                                {"googleClientSecret",
	                                                 "google_client_secret",
	                                                 "client_secret"})
	                    : extract_first_json_string(contents,
	                                                {"googleClientSecret",
	                                                 "google_client_secret"}));
	assign_if_empty(&config->backend_base_url,
	                extract_first_json_string(contents, {"backendBaseUrl", "backend_base_url"}));
}

FirebaseAuthConfig normalize_config(FirebaseAuthConfig config)
{
	config.api_key = trim_copy(config.api_key);
	config.project_id = trim_copy(config.project_id);
	config.auth_domain = trim_copy(config.auth_domain);
	config.storage_bucket = trim_copy(config.storage_bucket);
	config.google_client_id = trim_copy(config.google_client_id);
	config.google_client_secret = trim_copy(config.google_client_secret);
	config.backend_base_url = normalize_backend_base_url(config.backend_base_url);
	if (config.auth_domain.empty() && !config.project_id.empty()) {
		config.auth_domain = config.project_id + ".firebaseapp.com";
	}
	if (config.storage_bucket.empty() && !config.project_id.empty()) {
		config.storage_bucket = config.project_id + ".firebasestorage.app";
	}
	return config;
}

std::string humanize_firebase_error(const std::string &error_code)
{
	if (error_code == "INVALID_LOGIN_CREDENTIALS" ||
	    error_code == "INVALID_PASSWORD" ||
	    error_code == "EMAIL_NOT_FOUND") {
		return "Invalid email or password.";
	}
	if (error_code == "EMAIL_EXISTS") {
		return "An account with this email already exists.";
	}
	if (error_code == "OPERATION_NOT_ALLOWED") {
		return "Email/password authentication is not enabled for this Firebase project.";
	}
	if (error_code == "USER_DISABLED") {
		return "This Firebase user has been disabled.";
	}
	if (error_code == "TOO_MANY_ATTEMPTS_TRY_LATER") {
		return "Too many login attempts. Try again later.";
	}
	if (error_code == "TOKEN_EXPIRED" || error_code == "INVALID_REFRESH_TOKEN") {
		return "The saved Firebase session has expired. Sign in again.";
	}
	if (error_code == "CONFIGURATION_NOT_FOUND") {
		return "Firebase Authentication could not find a valid sign-in configuration for this request. "
		       "Check that the API key and project ID belong to the same Firebase project and that "
		       "Email/Password sign-in is enabled for that project.";
	}
	if (error_code == "PROJECT_NOT_FOUND") {
		return "The Firebase project could not be found for the supplied API key.";
	}
	if (error_code == "API_KEY_INVALID") {
		return "The Firebase API key is invalid.";
	}
	if (error_code == "ACCOUNT_EXISTS_WITH_DIFFERENT_CREDENTIAL") {
		return "That Google account already exists with a different sign-in method for this Firebase project.";
	}
	if (error_code == "INVALID_IDP_RESPONSE" || error_code == "INVALID_IDP_CREDENTIAL") {
		return "Google sign-in returned an invalid identity token.";
	}
	if (!error_code.empty()) {
		return error_code;
	}
	return "Firebase authentication failed.";
}

std::string parse_firebase_error_message(const std::string &body)
{
	return humanize_firebase_error(extract_first_json_string(body, {"message"}));
}

size_t curl_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	if (userdata == nullptr) {
		return 0;
	}
	std::string *buffer = static_cast<std::string *>(userdata);
	buffer->append(ptr, size * nmemb);
	return size * nmemb;
}

bool ensure_curl_initialized(std::string *error)
{
	std::call_once(g_curl_init_once, []() {
		const CURLcode result = curl_global_init(CURL_GLOBAL_DEFAULT);
		g_curl_initialized = (result == CURLE_OK);
		if (!g_curl_initialized) {
			g_curl_init_error = curl_easy_strerror(result);
		}
	});
	if (!g_curl_initialized && error != nullptr) {
		*error = g_curl_init_error.empty() ? "Failed to initialize libcurl." : g_curl_init_error;
	}
	return g_curl_initialized;
}

bool perform_post_request(const std::string &url,
                          const std::string &payload,
                          const char *content_type,
                          HttpResponse *response)
{
	if (response == nullptr) {
		return false;
	}
	std::string curl_error;
	if (!ensure_curl_initialized(&curl_error)) {
		response->error = curl_error;
		return false;
	}

	CURL *curl = curl_easy_init();
	if (curl == nullptr) {
		response->error = "Unable to create libcurl request handle.";
		return false;
	}

	std::array<char, CURL_ERROR_SIZE> curl_error_buffer{};
	curl_error_buffer[0] = '\0';

	struct curl_slist *headers = nullptr;
	std::string content_type_header = std::string("Content-Type: ") + content_type;
	headers = curl_slist_append(headers, content_type_header.c_str());

	response->body.clear();
	response->error.clear();
	response->status_code = 0;

	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error_buffer.data());
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(payload.size()));
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response->body);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Progen3d FirebaseAuth/1.0");
	if (!firebase_auth_configure_curl_tls(curl, &response->error)) {
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
		return false;
	}

	const CURLcode result = curl_easy_perform(curl);
	if (result != CURLE_OK) {
		response->error =
			build_ssl_diagnostics_message(curl, result, curl_error_buffer.data(), url);
		curl_slist_free_all(headers);
		curl_easy_cleanup(curl);
		return false;
	}

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->status_code);
	curl_slist_free_all(headers);
	curl_easy_cleanup(curl);
	return true;
}

std::string url_encode(const std::string &value)
{
	std::string init_error;
	if (!ensure_curl_initialized(&init_error)) {
		return "";
	}
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

std::string url_decode(const std::string &value)
{
	std::string output;
	output.reserve(value.size());
	for (std::size_t index = 0; index < value.size(); ++index) {
		const char ch = value[index];
		if (ch == '+') {
			output.push_back(' ');
			continue;
		}
		if (ch == '%' && index + 2 < value.size()) {
			const char hex[] = {value[index + 1], value[index + 2], '\0'};
			char *end = nullptr;
			const long decoded = std::strtol(hex, &end, 16);
			if (end != nullptr && *end == '\0') {
				output.push_back(static_cast<char>(decoded));
				index += 2;
				continue;
			}
		}
		output.push_back(ch);
	}
	return output;
}

std::unordered_map<std::string, std::string> parse_url_query_pairs(const std::string &query)
{
	std::unordered_map<std::string, std::string> pairs;
	std::size_t start = 0;
	while (start < query.size()) {
		const std::size_t separator = query.find('&', start);
		const std::string entry = query.substr(start,
		                                       separator == std::string::npos
		                                           ? std::string::npos
		                                           : separator - start);
		const std::size_t equals = entry.find('=');
		const std::string key = url_decode(entry.substr(0, equals));
		const std::string value =
			equals == std::string::npos ? "" : url_decode(entry.substr(equals + 1));
		if (!key.empty()) {
			pairs[key] = value;
		}
		if (separator == std::string::npos) {
			break;
		}
		start = separator + 1;
	}
	return pairs;
}

std::string random_url_safe_string(std::size_t length)
{
	static const char kAlphabet[] =
		"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~";
	std::random_device random_device;
	std::mt19937 generator(random_device());
	std::uniform_int_distribution<std::size_t> distribution(0, sizeof(kAlphabet) - 2);
	std::string value;
	value.reserve(length);
	for (std::size_t index = 0; index < length; ++index) {
		value.push_back(kAlphabet[distribution(generator)]);
	}
	return value;
}

bool create_loopback_listener(int *listener_fd, int *port, std::string *error)
{
	if (listener_fd == nullptr || port == nullptr) {
		if (error != nullptr) {
			*error = "Google sign-in could not allocate a local callback listener.";
		}
		return false;
	}

	const int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		if (error != nullptr) {
			*error = "Google sign-in could not open a local callback socket.";
		}
		return false;
	}

	int reuse_address = 1;
	setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(reuse_address));

	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	address.sin_port = htons(0);
	if (bind(socket_fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) != 0) {
		close(socket_fd);
		if (error != nullptr) {
			*error = "Google sign-in could not bind the local callback socket.";
		}
		return false;
	}
	if (listen(socket_fd, 1) != 0) {
		close(socket_fd);
		if (error != nullptr) {
			*error = "Google sign-in could not listen for the browser callback.";
		}
		return false;
	}

	sockaddr_in bound_address{};
	socklen_t bound_length = sizeof(bound_address);
	if (getsockname(socket_fd, reinterpret_cast<sockaddr *>(&bound_address), &bound_length) != 0) {
		close(socket_fd);
		if (error != nullptr) {
			*error = "Google sign-in could not determine the local callback port.";
		}
		return false;
	}

	*listener_fd = socket_fd;
	*port = ntohs(bound_address.sin_port);
	return true;
}

void send_loopback_browser_response(int client_fd, bool success)
{
	if (client_fd < 0) {
		return;
	}
	const std::string body =
		success
			? "<html><body style='font-family:sans-serif;padding:24px;'><h2>Google sign-in complete</h2><p>You can return to Progen3d.</p></body></html>"
			: "<html><body style='font-family:sans-serif;padding:24px;'><h2>Google sign-in failed</h2><p>Return to Progen3d and try again.</p></body></html>";
	std::ostringstream response;
	response << "HTTP/1.1 200 OK\r\n"
	         << "Content-Type: text/html; charset=utf-8\r\n"
	         << "Content-Length: " << body.size() << "\r\n"
	         << "Connection: close\r\n\r\n"
	         << body;
	const std::string payload = response.str();
	send(client_fd, payload.data(), payload.size(), 0);
}

bool wait_for_google_browser_callback(int listener_fd,
                                      const std::string &expected_state,
                                      std::string *authorization_code,
                                      std::string *error)
{
	if (listener_fd < 0) {
		if (error != nullptr) {
			*error = "Google sign-in listener is invalid.";
		}
		return false;
	}

	fd_set read_set;
	FD_ZERO(&read_set);
	FD_SET(listener_fd, &read_set);
	timeval timeout{};
	timeout.tv_sec = 180;
	timeout.tv_usec = 0;
	const int select_result = select(listener_fd + 1, &read_set, nullptr, nullptr, &timeout);
	if (select_result <= 0) {
		if (error != nullptr) {
			*error = select_result == 0
			             ? "Google sign-in timed out waiting for the browser callback."
			             : "Google sign-in failed while waiting for the browser callback.";
		}
		return false;
	}

	sockaddr_in client_address{};
	socklen_t client_length = sizeof(client_address);
	const int client_fd =
		accept(listener_fd, reinterpret_cast<sockaddr *>(&client_address), &client_length);
	if (client_fd < 0) {
		if (error != nullptr) {
			*error = "Google sign-in could not accept the browser callback.";
		}
		return false;
	}

	char buffer[8192];
	const ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
	if (bytes_read <= 0) {
		send_loopback_browser_response(client_fd, false);
		close(client_fd);
		if (error != nullptr) {
			*error = "Google sign-in received an empty browser callback.";
		}
		return false;
	}
	buffer[bytes_read] = '\0';
	const std::string request(buffer);
	const std::size_t request_line_end = request.find("\r\n");
	const std::string request_line =
		request.substr(0, request_line_end == std::string::npos ? request.size() : request_line_end);
	const std::size_t path_start = request_line.find(' ');
	const std::size_t path_end =
		path_start == std::string::npos ? std::string::npos : request_line.find(' ', path_start + 1);
	const std::string target =
		path_start == std::string::npos
			? ""
			: request_line.substr(path_start + 1,
			                      path_end == std::string::npos ? std::string::npos : path_end - path_start - 1);
	const std::size_t query_marker = target.find('?');
	const std::string query = query_marker == std::string::npos ? "" : target.substr(query_marker + 1);
	const std::unordered_map<std::string, std::string> query_pairs = parse_url_query_pairs(query);
	const auto state_it = query_pairs.find("state");
	if (state_it == query_pairs.end() || state_it->second != expected_state) {
		send_loopback_browser_response(client_fd, false);
		close(client_fd);
		if (error != nullptr) {
			*error = "Google sign-in callback state did not match.";
		}
		return false;
	}
	const auto error_it = query_pairs.find("error");
	if (error_it != query_pairs.end() && !error_it->second.empty()) {
		send_loopback_browser_response(client_fd, false);
		close(client_fd);
		if (error != nullptr) {
			*error = "Google sign-in was cancelled or denied: " + error_it->second;
		}
		return false;
	}
	const auto code_it = query_pairs.find("code");
	if (code_it == query_pairs.end() || code_it->second.empty()) {
		send_loopback_browser_response(client_fd, false);
		close(client_fd);
		if (error != nullptr) {
			*error = "Google sign-in callback did not include an authorization code.";
		}
		return false;
	}

	send_loopback_browser_response(client_fd, true);
	close(client_fd);
	if (authorization_code != nullptr) {
		*authorization_code = code_it->second;
	}
	return true;
}

bool exchange_google_authorization_code(const FirebaseAuthConfig &config,
                                        const std::string &authorization_code,
                                        const std::string &redirect_uri,
                                        const std::string &code_verifier,
                                        std::string *google_id_token,
                                        std::string *google_access_token,
                                        std::string *error)
{
	std::ostringstream payload;
	payload << "client_id=" << url_encode(config.google_client_id)
	        << "&code=" << url_encode(authorization_code)
	        << "&code_verifier=" << url_encode(code_verifier)
	        << "&grant_type=authorization_code"
	        << "&redirect_uri=" << url_encode(redirect_uri);
	if (!config.google_client_secret.empty()) {
		payload << "&client_secret=" << url_encode(config.google_client_secret);
	}

	HttpResponse response;
	if (!perform_post_request("https://oauth2.googleapis.com/token",
	                          payload.str(),
	                          "application/x-www-form-urlencoded",
	                          &response)) {
		if (error != nullptr) {
			*error = response.error.empty() ? "Unable to reach Google sign-in." : response.error;
		}
		return false;
	}
	if (response.status_code < 200 || response.status_code >= 300) {
		if (error != nullptr) {
			*error = extract_first_json_string(response.body, {"error_description", "error"});
			if (error->find("client_secret is missing") != std::string::npos ||
			    error->find("client secret is missing") != std::string::npos) {
				*error =
					"Google sign-in needs googleClientSecret for this OAuth client. "
					"Add googleClientSecret to firebase_auth_config.json, or use a Google OAuth desktop client ID instead.";
			}
			if (error->empty()) {
				*error = "Google sign-in failed while exchanging the authorization code.";
			}
		}
		return false;
	}

	const std::string id_token = extract_first_json_string(response.body, {"id_token"});
	const std::string access_token = extract_first_json_string(response.body, {"access_token"});
	if (id_token.empty() && access_token.empty()) {
		if (error != nullptr) {
			*error = "Google sign-in returned neither an ID token nor an access token.";
		}
		return false;
	}

	if (google_id_token != nullptr) {
		*google_id_token = id_token;
	}
	if (google_access_token != nullptr) {
		*google_access_token = access_token;
	}
	return true;
}

bool exchange_google_token_for_firebase_session(const FirebaseAuthConfig &config,
                                                const std::string &google_id_token,
                                                const std::string &google_access_token,
                                                FirebaseAuthSession *session,
                                                std::string *error)
{
	std::string post_body;
	if (!google_id_token.empty()) {
		post_body += "id_token=" + url_encode(google_id_token);
	}
	if (!google_access_token.empty()) {
		if (!post_body.empty()) {
			post_body += "&";
		}
		post_body += "access_token=" + url_encode(google_access_token);
	}
	if (!post_body.empty()) {
		post_body += "&";
	}
	post_body += "providerId=google.com";

	std::ostringstream payload;
	payload << "{"
	        << "\"postBody\":\"" << json_escape(post_body) << "\","
	        << "\"requestUri\":\"http://localhost\","
	        << "\"returnSecureToken\":true,"
	        << "\"returnIdpCredential\":true"
	        << "}";

	const std::string url =
		"https://identitytoolkit.googleapis.com/v1/accounts:signInWithIdp?key=" + config.api_key;
	return run_auth_exchange(url, payload.str(), "application/json", session, error);
}

std::string shell_escape_single_quoted(const std::string &value)
{
	std::string escaped;
	escaped.reserve(value.size() + 8);
	for (char ch : value) {
		if (ch == '\'') {
			escaped += "'\\''";
		} else {
			escaped.push_back(ch);
		}
	}
	return escaped;
}

bool parse_auth_session_from_json(const std::string &body,
                                  FirebaseAuthSession *session)
{
	if (session == nullptr) {
		return false;
	}
	const std::string id_token = extract_first_json_string(body, {"idToken", "id_token"});
	const std::string refresh_token = extract_first_json_string(body, {"refreshToken", "refresh_token"});
	if (id_token.empty() || refresh_token.empty()) {
		return false;
	}
	session->authenticated = true;
	session->id_token = id_token;
	session->refresh_token = refresh_token;
	const std::string email = extract_first_json_string(body, {"email"});
	if (!email.empty()) {
		session->email = email;
	}
	const std::string local_id = extract_first_json_string(body, {"localId", "user_id"});
	if (!local_id.empty()) {
		session->local_id = local_id;
	}
	const std::string expires_in = extract_first_json_string(body, {"expiresIn", "expires_in"});
	if (!expires_in.empty()) {
		try {
			session->expires_in_seconds = std::stoi(expires_in);
		} catch (...) {
			session->expires_in_seconds = 0;
		}
	}
	return true;
}

std::string build_email_password_payload(const std::string &email,
                                         const std::string &password)
{
	std::ostringstream payload;
	payload << "{"
	        << "\"email\":\"" << json_escape(email) << "\","
	        << "\"password\":\"" << json_escape(password) << "\","
	        << "\"returnSecureToken\":true"
	        << "}";
	return payload.str();
}

bool run_auth_exchange(const std::string &url,
                       const std::string &payload,
                       const char *content_type,
                       FirebaseAuthSession *session,
                       std::string *error)
{
	HttpResponse response;
	if (!perform_post_request(url, payload, content_type, &response)) {
		if (error != nullptr) {
			*error = response.error.empty() ? "Unable to reach Firebase Authentication." : response.error;
		}
		return false;
	}
	if (response.status_code < 200 || response.status_code >= 300) {
		if (error != nullptr) {
			const std::string parsed = parse_firebase_error_message(response.body);
			*error = parsed.empty()
			             ? ("Firebase request failed with HTTP " + std::to_string(response.status_code))
			             : parsed;
		}
		return false;
	}
	if (!parse_auth_session_from_json(response.body, session)) {
		if (error != nullptr) {
			*error = "Firebase returned an unexpected authentication response.";
		}
		return false;
	}
	return true;
}

}  // namespace

bool initialize_firebase_auth_support(std::string *error)
{
	if (!ensure_curl_initialized(error)) {
		return false;
	}
	std::call_once(g_curl_tls_once, []() { initialize_curl_tls_paths(); });
	return true;
}

bool firebase_auth_configure_curl_tls(CURL *curl, std::string *error)
{
	if (curl == nullptr) {
		if (error != nullptr) {
			*error = "TLS setup failed: curl handle is null.";
		}
		return false;
	}
	if (!initialize_firebase_auth_support(error)) {
		return false;
	}

	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
	const std::string env_cainfo = getenv_string("PROGEN3D_CURL_CAINFO");
	const std::string env_capath = getenv_string("PROGEN3D_CURL_CAPATH");
	if (!env_cainfo.empty()) {
		curl_easy_setopt(curl, CURLOPT_CAINFO, env_cainfo.c_str());
	} else if (!g_curl_ca_file.empty()) {
		curl_easy_setopt(curl, CURLOPT_CAINFO, g_curl_ca_file.c_str());
	}
	if (!env_capath.empty()) {
		curl_easy_setopt(curl, CURLOPT_CAPATH, env_capath.c_str());
	} else if (!g_curl_ca_path.empty()) {
		curl_easy_setopt(curl, CURLOPT_CAPATH, g_curl_ca_path.c_str());
	}
	return true;
}

std::string firebase_auth_build_curl_error_message(CURL *curl,
                                                   CURLcode result,
                                                   const char *error_buffer,
                                                   const std::string &url)
{
	return build_ssl_diagnostics_message(curl, result, error_buffer, url);
}

void shutdown_firebase_auth_support()
{
	if (g_curl_initialized) {
		curl_global_cleanup();
		g_curl_initialized = false;
	}
	g_curl_ca_file.clear();
	g_curl_ca_path.clear();
}

const char *firebase_auth_default_config_path()
{
	static std::string path;
	path = firebase_auth_config_state_path().string();
	return path.c_str();
}

const char *firebase_auth_default_session_path()
{
	static std::string path;
	path = firebase_auth_session_state_path().string();
	return path.c_str();
}

bool load_firebase_auth_config(FirebaseAuthConfig *config, std::string *error)
{
	if (config == nullptr) {
		if (error != nullptr) {
			*error = "FirebaseAuthConfig output was null.";
		}
		return false;
	}

	*config = FirebaseAuthConfig{};

	std::string contents;
	const std::filesystem::path state_config_path = firebase_auth_config_state_path();
	const std::filesystem::path bundled_config_path = firebase_auth_bundled_config_path();
	std::filesystem::path loaded_config_path;
	if (read_text_file(state_config_path, &contents)) {
		loaded_config_path = state_config_path;
	} else if (read_text_file(bundled_config_path, &contents)) {
		loaded_config_path = bundled_config_path;
	}
	if (!loaded_config_path.empty()) {
		merge_firebase_auth_config_from_json(config, contents, true);
		config->loaded_from = loaded_config_path.string();
	}

	std::filesystem::path supplemental_oauth_path;
	for (const char *candidate : kGoogleServicesCandidates) {
		const std::filesystem::path candidate_path = firebase_auth_bundled_google_services_path(candidate);
		if (candidate == nullptr || !read_text_file(candidate_path, &contents)) {
			continue;
		}
		const FirebaseAuthConfig before_merge = *config;
		merge_firebase_auth_config_from_json(config,
		                                     contents,
		                                     std::string(candidate) == "google-services-desktop.json");
		if (supplemental_oauth_path.empty() &&
		    (before_merge.google_client_id != config->google_client_id ||
		     before_merge.google_client_secret != config->google_client_secret)) {
			supplemental_oauth_path = candidate_path;
		}
	}

	*config = normalize_config(*config);
	if (config->valid()) {
		if (config->loaded_from.empty()) {
			if (!supplemental_oauth_path.empty()) {
				config->loaded_from = supplemental_oauth_path.string();
			}
		} else if (!supplemental_oauth_path.empty() &&
		           config->loaded_from != supplemental_oauth_path.string()) {
			config->loaded_from += " + " + supplemental_oauth_path.string();
		}
		return true;
	}

	if (error != nullptr) {
		*error = !loaded_config_path.empty()
		             ? "firebase_auth_config.json is missing apiKey or projectId."
		             : "No Firebase auth config was found.";
	}
	return false;
}

bool save_firebase_auth_config(const FirebaseAuthConfig &config, std::string *error)
{
	const FirebaseAuthConfig normalized = normalize_config(config);
	if (!normalized.valid()) {
		if (error != nullptr) {
			*error = "Firebase config needs both apiKey and projectId.";
		}
		return false;
	}

	std::ostringstream json;
	json << "{\n"
	     << "  \"apiKey\": \"" << json_escape(normalized.api_key) << "\",\n"
	     << "  \"projectId\": \"" << json_escape(normalized.project_id) << "\",\n"
	     << "  \"authDomain\": \"" << json_escape(normalized.auth_domain) << "\",\n"
	     << "  \"storageBucket\": \"" << json_escape(normalized.storage_bucket) << "\",\n"
	     << "  \"googleClientId\": \"" << json_escape(normalized.google_client_id) << "\",\n"
	     << "  \"googleClientSecret\": \"" << json_escape(normalized.google_client_secret) << "\",\n"
	     << "  \"backendBaseUrl\": \"" << json_escape(normalized.backend_base_url) << "\"\n"
	     << "}\n";
	return write_text_file(firebase_auth_config_state_path(), json.str(), error);
}

bool load_firebase_auth_session(FirebaseAuthSession *session, std::string *error)
{
	if (session == nullptr) {
		if (error != nullptr) {
			*error = "FirebaseAuthSession output was null.";
		}
		return false;
	}
	*session = FirebaseAuthSession{};

	std::string contents;
	if (!read_text_file(firebase_auth_session_state_path(), &contents)) {
		if (error != nullptr) {
			*error = "No saved Firebase session was found.";
		}
		return false;
	}

	session->email = extract_first_json_string(contents, {"email"});
	session->local_id = extract_first_json_string(contents, {"localId", "local_id"});
	session->refresh_token = extract_first_json_string(contents, {"refreshToken", "refresh_token"});
	session->id_token = extract_first_json_string(contents, {"idToken", "id_token"});
	const std::string expires_in = extract_first_json_string(contents, {"expiresIn", "expires_in"});
	if (!expires_in.empty()) {
		try {
			session->expires_in_seconds = std::stoi(expires_in);
		} catch (...) {
			session->expires_in_seconds = 0;
		}
	}
	session->authenticated = !session->refresh_token.empty();
	if (!session->authenticated) {
		if (error != nullptr) {
			*error = "Saved Firebase session is missing a refresh token.";
		}
		return false;
	}
	return true;
}

bool save_firebase_auth_session(const FirebaseAuthSession &session, std::string *error)
{
	if (session.refresh_token.empty()) {
		if (error != nullptr) {
			*error = "Firebase session is missing a refresh token.";
		}
		return false;
	}

	std::ostringstream json;
	json << "{\n"
	     << "  \"email\": \"" << json_escape(session.email) << "\",\n"
	     << "  \"localId\": \"" << json_escape(session.local_id) << "\",\n"
	     << "  \"idToken\": \"" << json_escape(session.id_token) << "\",\n"
	     << "  \"refreshToken\": \"" << json_escape(session.refresh_token) << "\",\n"
	     << "  \"expiresIn\": \"" << session.expires_in_seconds << "\"\n"
	     << "}\n";
	return write_text_file(firebase_auth_session_state_path(), json.str(), error);
}

void clear_firebase_auth_session_file()
{
	std::error_code fs_error;
	std::filesystem::remove(firebase_auth_session_state_path(), fs_error);
}

bool open_url_in_browser(const std::string &url, std::string *error)
{
	if (url.empty()) {
		if (error != nullptr) {
			*error = "The browser URL is empty.";
		}
		return false;
	}
#if defined(__APPLE__)
	const std::string command =
		"open '" + shell_escape_single_quoted(url) + "' >/dev/null 2>&1 &";
#elif defined(_WIN32)
	const std::string command =
		"start \"\" \"" + url + "\"";
#else
	const std::string command =
		"xdg-open '" + shell_escape_single_quoted(url) + "' >/dev/null 2>&1 &";
#endif
	const int result = std::system(command.c_str());
	if (result != 0) {
		if (error != nullptr) {
			*error = "Unable to open the browser for " + url;
		}
		return false;
	}
	return true;
}

bool firebase_auth_sign_in(const FirebaseAuthConfig &config,
                           const std::string &email,
                           const std::string &password,
                           FirebaseAuthSession *session,
                           std::string *error)
{
	const FirebaseAuthConfig normalized = normalize_config(config);
	if (!normalized.valid()) {
		if (error != nullptr) {
			*error = "Firebase config is incomplete.";
		}
		return false;
	}
	if (email.empty() || password.empty()) {
		if (error != nullptr) {
			*error = "Email and password are required.";
		}
		return false;
	}

	FirebaseAuthSession next_session;
	next_session.email = email;
	const std::string url =
		"https://identitytoolkit.googleapis.com/v1/accounts:signInWithPassword?key=" + normalized.api_key;
	if (!run_auth_exchange(url,
	                       build_email_password_payload(email, password),
	                       "application/json",
	                       &next_session,
	                       error)) {
		return false;
	}
	if (session != nullptr) {
		*session = next_session;
	}
	return true;
}

bool firebase_auth_sign_in_with_google(const FirebaseAuthConfig &config,
                                       FirebaseAuthSession *session,
                                       std::string *error)
{
	const FirebaseAuthConfig normalized = normalize_config(config);
	if (!normalized.valid()) {
		if (error != nullptr) {
			*error = "Firebase config is incomplete.";
		}
		return false;
	}
	if (normalized.google_client_id.empty()) {
		if (error != nullptr) {
			*error =
				"Google sign-in needs a Google OAuth client ID in googleClientId. "
				"Desktop client IDs work as-is; web client IDs also need googleClientSecret.";
		}
		return false;
	}

	int listener_fd = -1;
	int callback_port = 0;
	if (!create_loopback_listener(&listener_fd, &callback_port, error)) {
		return false;
	}

	const std::string state = random_url_safe_string(32);
	const std::string code_verifier = random_url_safe_string(64);
	const std::string redirect_uri =
		"http://127.0.0.1:" + std::to_string(callback_port) + "/oauth2callback";
	const std::string auth_url =
		"https://accounts.google.com/o/oauth2/v2/auth"
		"?client_id=" + url_encode(normalized.google_client_id) +
		"&redirect_uri=" + url_encode(redirect_uri) +
		"&response_type=code"
		"&scope=" + url_encode("openid email profile") +
		"&prompt=select_account"
		"&state=" + url_encode(state) +
		"&code_challenge=" + url_encode(code_verifier) +
		"&code_challenge_method=plain";

	std::string browser_error;
	if (!open_url_in_browser(auth_url, &browser_error)) {
		close(listener_fd);
		if (error != nullptr) {
			*error = browser_error;
		}
		return false;
	}

	std::string authorization_code;
	const bool received_callback =
		wait_for_google_browser_callback(listener_fd, state, &authorization_code, error);
	close(listener_fd);
	if (!received_callback) {
		return false;
	}

	std::string google_id_token;
	std::string google_access_token;
	if (!exchange_google_authorization_code(normalized,
	                                        authorization_code,
	                                        redirect_uri,
	                                        code_verifier,
	                                        &google_id_token,
	                                        &google_access_token,
	                                        error)) {
		return false;
	}

	FirebaseAuthSession next_session;
	if (!exchange_google_token_for_firebase_session(normalized,
	                                                google_id_token,
	                                                google_access_token,
	                                                &next_session,
	                                                error)) {
		return false;
	}
	if (session != nullptr) {
		*session = next_session;
	}
	return true;
}

bool firebase_auth_sign_up(const FirebaseAuthConfig &config,
                           const std::string &email,
                           const std::string &password,
                           FirebaseAuthSession *session,
                           std::string *error)
{
	const FirebaseAuthConfig normalized = normalize_config(config);
	if (!normalized.valid()) {
		if (error != nullptr) {
			*error = "Firebase config is incomplete.";
		}
		return false;
	}
	if (email.empty() || password.empty()) {
		if (error != nullptr) {
			*error = "Email and password are required.";
		}
		return false;
	}

	FirebaseAuthSession next_session;
	next_session.email = email;
	const std::string url =
		"https://identitytoolkit.googleapis.com/v1/accounts:signUp?key=" + normalized.api_key;
	if (!run_auth_exchange(url,
	                       build_email_password_payload(email, password),
	                       "application/json",
	                       &next_session,
	                       error)) {
		return false;
	}
	if (session != nullptr) {
		*session = next_session;
	}
	return true;
}

bool firebase_auth_refresh(const FirebaseAuthConfig &config,
                           FirebaseAuthSession *session,
                           std::string *error)
{
	const FirebaseAuthConfig normalized = normalize_config(config);
	if (!normalized.valid()) {
		if (error != nullptr) {
			*error = "Firebase config is incomplete.";
		}
		return false;
	}
	if (session == nullptr || session->refresh_token.empty()) {
		if (error != nullptr) {
			*error = "No Firebase refresh token is available.";
		}
		return false;
	}

	std::string curl_error;
	if (!ensure_curl_initialized(&curl_error)) {
		if (error != nullptr) {
			*error = curl_error;
		}
		return false;
	}

	CURL *curl = curl_easy_init();
	if (curl == nullptr) {
		if (error != nullptr) {
			*error = "Unable to create libcurl request handle.";
		}
		return false;
	}

	char *escaped_token = curl_easy_escape(curl, session->refresh_token.c_str(), 0);
	if (escaped_token == nullptr) {
		curl_easy_cleanup(curl);
		if (error != nullptr) {
			*error = "Unable to encode the Firebase refresh token.";
		}
		return false;
	}
	const std::string payload =
		"grant_type=refresh_token&refresh_token=" + std::string(escaped_token);
	curl_free(escaped_token);
	curl_easy_cleanup(curl);

	FirebaseAuthSession next_session = *session;
	const std::string url =
		"https://securetoken.googleapis.com/v1/token?key=" + normalized.api_key;
	if (!run_auth_exchange(url,
	                       payload,
	                       "application/x-www-form-urlencoded",
	                       &next_session,
	                       error)) {
		return false;
	}
	if (session != nullptr) {
		*session = next_session;
	}
	return true;
}
