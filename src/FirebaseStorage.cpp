#include "FirebaseStorage.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <curl/curl.h>

namespace {

constexpr const char *kFirebaseStorageEndpoint = "https://firebasestorage.googleapis.com";
constexpr const char *kPrivateGrammarPrefix = "grammars";
constexpr const char *kGalleryGrammarPrefix = "gallery";

struct HttpResponse {
	long status_code = 0;
	std::string body;
	std::string error;
};

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

std::string sanitize_grammar_name(const std::string &name)
{
	std::string value = trim_copy(name);
	if (value.empty()) {
		return "";
	}
	for (char &ch : value) {
		if (ch == '\\') {
			ch = '/';
		}
	}
	const std::size_t slash = value.find_last_of('/');
	if (slash != std::string::npos) {
		value = value.substr(slash + 1);
	}
	if (value == "." || value == "..") {
		return "";
	}
	if (value.size() < 8 || value.substr(value.size() - 8) != ".grammar") {
		value += ".grammar";
	}
	return value;
}

bool valid_grammar_name(const std::string &name)
{
	if (name.empty() || name == "." || name == "..") {
		return false;
	}
	for (char ch : name) {
		if (ch == '/' || ch == '\\' || ch == '\n' || ch == '\r') {
			return false;
		}
	}
	return true;
}

std::string default_storage_bucket_for_project(const std::string &project_id)
{
	return project_id.empty() ? "" : (project_id + ".firebasestorage.app");
}

std::vector<std::string> storage_bucket_candidates(const FirebaseAuthConfig &config)
{
	std::vector<std::string> buckets;
	if (!config.storage_bucket.empty()) {
		buckets.push_back(config.storage_bucket);
	}
	if (!config.project_id.empty()) {
		const std::string default_bucket = default_storage_bucket_for_project(config.project_id);
		if (!default_bucket.empty() &&
		    std::find(buckets.begin(), buckets.end(), default_bucket) == buckets.end()) {
			buckets.push_back(default_bucket);
		}
		const std::string legacy_bucket = config.project_id + ".appspot.com";
		if (std::find(buckets.begin(), buckets.end(), legacy_bucket) == buckets.end()) {
			buckets.push_back(legacy_bucket);
		}
	}
	return buckets;
}

std::string storage_user_prefix(const FirebaseAuthSession &session, const char *root_prefix)
{
	return std::string(root_prefix) + "/" + session.local_id + "/";
}

std::string private_grammar_object_path(const FirebaseAuthSession &session,
                                        const std::string &grammar_name)
{
	return storage_user_prefix(session, kPrivateGrammarPrefix) + grammar_name;
}

std::string published_grammar_object_path(const FirebaseAuthSession &session,
                                          const std::string &grammar_name)
{
	return storage_user_prefix(session, kGalleryGrammarPrefix) + grammar_name;
}

std::string published_metadata_object_path(const FirebaseAuthSession &session,
                                           const std::string &grammar_name)
{
	const std::size_t extension = grammar_name.find_last_of('.');
	const std::string stem =
		extension == std::string::npos ? grammar_name : grammar_name.substr(0, extension);
	return storage_user_prefix(session, kGalleryGrammarPrefix) + stem + ".json";
}

std::string current_iso8601_timestamp()
{
	std::time_t now = std::time(nullptr);
	std::tm utc_time{};
#if defined(_WIN32)
	gmtime_s(&utc_time, &now);
#else
	gmtime_r(&now, &utc_time);
#endif
	char buffer[32];
	if (std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &utc_time) == 0) {
		return "";
	}
	return buffer;
}

std::string extract_json_string_for_key(const std::string &text, const std::string &key)
{
	const std::string needle = "\"" + key + "\"";
	std::size_t position = 0;
	while ((position = text.find(needle, position)) != std::string::npos) {
		std::size_t colon = text.find(':', position + needle.size());
		if (colon == std::string::npos) {
			return "";
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
		return "";
	}
	return "";
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

bool prepare_storage_session(const FirebaseAuthConfig &config,
                             FirebaseAuthSession *session,
                             std::string *error)
{
	if (!config.valid()) {
		if (error != nullptr) {
			*error = "Firebase config is incomplete.";
		}
		return false;
	}
	if (session == nullptr || !session->authenticated || session->local_id.empty()) {
		if (error != nullptr) {
			*error = "Sign in to Firebase before using cloud storage.";
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

bool perform_request(const std::string &method,
                     const std::string &url,
                     const std::string &body,
                     const std::string &content_type,
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
	if (!content_type.empty()) {
		const std::string content_type_header = "Content-Type: " + content_type;
		curl_headers = curl_slist_append(curl_headers, content_type_header.c_str());
	}
	for (const std::string &header : headers) {
		curl_headers = curl_slist_append(curl_headers, header.c_str());
	}

	response->body.clear();
	response->error.clear();
	response->status_code = 0;

	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error_buffer.data());
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_callback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response->body);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 25L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Progen3d FirebaseStorage/1.0");
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

std::string url_encode(const std::string &value)
{
	std::string init_error;
	if (!initialize_firebase_auth_support(&init_error)) {
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

std::string firebase_error_message_from_body(const std::string &body)
{
	const std::string message = extract_json_string_for_key(body, "message");
	if (!message.empty()) {
		return message;
	}
	return trim_copy(body);
}

bool authorized_storage_request(const FirebaseAuthConfig &config,
                                FirebaseAuthSession *session,
                                const std::string &method,
                                const std::string &url,
                                const std::string &body,
                                const std::string &content_type,
                                HttpResponse *response,
                                std::string *error)
{
	if (!prepare_storage_session(config, session, error)) {
		return false;
	}

	std::vector<std::string> auth_headers;
	if (!session->id_token.empty()) {
		auth_headers.push_back("Authorization: Bearer " + session->id_token);
		auth_headers.push_back("Authorization: Firebase " + session->id_token);
	}
	auth_headers.push_back("");

	for (std::size_t attempt = 0; attempt < auth_headers.size(); ++attempt) {
		std::vector<std::string> headers;
		if (!auth_headers[attempt].empty()) {
			headers.push_back(auth_headers[attempt]);
		}

		HttpResponse next_response;
		if (!perform_request(method, url, body, content_type, headers, &next_response)) {
			if (attempt + 1 == auth_headers.size()) {
				if (error != nullptr) {
					*error = next_response.error.empty()
					             ? "Unable to reach Firebase Storage."
					             : next_response.error;
				}
				return false;
			}
			continue;
		}

		if ((next_response.status_code == 401 || next_response.status_code == 403) &&
		    !session->refresh_token.empty() &&
		    !firebase_auth_refresh(config, session, error)) {
			if (attempt + 1 == auth_headers.size()) {
				if (error != nullptr && error->empty()) {
					*error = firebase_error_message_from_body(next_response.body);
				}
				return false;
			}
		}

		if ((next_response.status_code == 401 || next_response.status_code == 403) &&
		    !session->refresh_token.empty() &&
		    attempt == 0 &&
		    firebase_auth_refresh(config, session, error)) {
			std::string save_error;
			save_firebase_auth_session(*session, &save_error);
			auth_headers[0] = "Authorization: Bearer " + session->id_token;
			auth_headers[1] = "Authorization: Firebase " + session->id_token;
			continue;
		}

		*response = std::move(next_response);
		return true;
	}

	if (error != nullptr && error->empty()) {
		*error = "Firebase Storage request failed.";
	}
	return false;
}

bool parse_storage_list_items(const std::string &body,
                              std::vector<FirebasePrivateGrammarRecord> *items)
{
	if (items == nullptr) {
		return false;
	}
	items->clear();

	const std::size_t items_key = body.find("\"items\"");
	if (items_key == std::string::npos) {
		return true;
	}
	const std::size_t array_start = body.find('[', items_key);
	if (array_start == std::string::npos) {
		return false;
	}
	std::size_t index = array_start + 1;
	while (index < body.size()) {
		while (index < body.size() &&
		       std::isspace(static_cast<unsigned char>(body[index])) != 0) {
			++index;
		}
		if (index >= body.size() || body[index] == ']') {
			break;
		}
		if (body[index] != '{') {
			++index;
			continue;
		}

		int depth = 0;
		bool in_string = false;
		bool escaped = false;
		const std::size_t object_start = index;
		for (; index < body.size(); ++index) {
			const char ch = body[index];
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
			if (ch == '{') {
				++depth;
				continue;
			}
			if (ch == '}') {
				--depth;
				if (depth == 0) {
					const std::string object_text =
						body.substr(object_start, index - object_start + 1);
					FirebasePrivateGrammarRecord item;
					item.object_path = extract_json_string_for_key(object_text, "name");
					const std::string size_value =
						extract_json_string_for_key(object_text, "size");
					item.updated = extract_json_string_for_key(object_text, "updated");
					if (!size_value.empty()) {
						try {
							item.size_bytes = static_cast<std::size_t>(std::stoull(size_value));
						} catch (...) {
							item.size_bytes = 0;
						}
					}
					if (!item.object_path.empty()) {
						const std::size_t slash = item.object_path.find_last_of('/');
						item.name =
							slash == std::string::npos ? item.object_path : item.object_path.substr(slash + 1);
						items->push_back(std::move(item));
					}
					++index;
					break;
				}
			}
		}
	}
	return true;
}

bool list_storage_prefix(const FirebaseAuthConfig &config,
                         FirebaseAuthSession *session,
                         const std::string &prefix,
                         std::vector<FirebasePrivateGrammarRecord> *items,
                         std::string *error)
{
	if (items == nullptr) {
		if (error != nullptr) {
			*error = "Storage list output was null.";
		}
		return false;
	}

	const std::vector<std::string> bucket_candidates = storage_bucket_candidates(config);
	if (bucket_candidates.empty()) {
		if (error != nullptr) {
			*error = "No Firebase Storage bucket is configured.";
		}
		return false;
	}

	const std::string encoded_prefix = url_encode(prefix);
	std::string last_error;
	for (const std::string &bucket : bucket_candidates) {
		const std::string url =
			std::string(kFirebaseStorageEndpoint) + "/v0/b/" + bucket + "/o?prefix=" + encoded_prefix;
		HttpResponse response;
		std::string request_error;
		if (!authorized_storage_request(config,
		                                session,
		                                "GET",
		                                url,
		                                "",
		                                "",
		                                &response,
		                                &request_error)) {
			last_error = request_error;
			continue;
		}
		if (response.status_code == 404) {
			last_error = "Firebase Storage bucket was not found.";
			continue;
		}
		if (response.status_code < 200 || response.status_code >= 300) {
			last_error = firebase_error_message_from_body(response.body);
			continue;
		}
		if (!parse_storage_list_items(response.body, items)) {
			last_error = "Firebase Storage returned an unreadable list response.";
			continue;
		}
		return true;
	}

	if (error != nullptr) {
		*error = last_error.empty() ? "Unable to list Firebase Storage objects." : last_error;
	}
	return false;
}

bool download_storage_text(const FirebaseAuthConfig &config,
                           FirebaseAuthSession *session,
                           const std::string &object_path,
                           std::string *contents,
                           std::string *error)
{
	if (contents == nullptr) {
		if (error != nullptr) {
			*error = "Storage download output was null.";
		}
		return false;
	}

	const std::vector<std::string> bucket_candidates = storage_bucket_candidates(config);
	const std::string encoded_object = url_encode(object_path);
	std::string last_error;
	for (const std::string &bucket : bucket_candidates) {
		const std::string url =
			std::string(kFirebaseStorageEndpoint) + "/v0/b/" + bucket + "/o/" + encoded_object + "?alt=media";
		HttpResponse response;
		std::string request_error;
		if (!authorized_storage_request(config,
		                                session,
		                                "GET",
		                                url,
		                                "",
		                                "",
		                                &response,
		                                &request_error)) {
			last_error = request_error;
			continue;
		}
		if (response.status_code == 404) {
			last_error = "The requested cloud grammar was not found.";
			continue;
		}
		if (response.status_code < 200 || response.status_code >= 300) {
			last_error = firebase_error_message_from_body(response.body);
			continue;
		}
		*contents = response.body;
		return true;
	}

	if (error != nullptr) {
		*error = last_error.empty() ? "Unable to download the cloud grammar." : last_error;
	}
	return false;
}

bool upload_storage_text(const FirebaseAuthConfig &config,
                         FirebaseAuthSession *session,
                         const std::string &object_path,
                         const std::string &contents,
                         const std::string &content_type,
                         std::string *error)
{
	const std::vector<std::string> bucket_candidates = storage_bucket_candidates(config);
	const std::string encoded_name = url_encode(object_path);
	std::string last_error;
	for (const std::string &bucket : bucket_candidates) {
		const std::string url =
			std::string(kFirebaseStorageEndpoint) + "/v0/b/" + bucket +
			"/o?uploadType=media&name=" + encoded_name;
		HttpResponse response;
		std::string request_error;
		if (!authorized_storage_request(config,
		                                session,
		                                "POST",
		                                url,
		                                contents,
		                                content_type,
		                                &response,
		                                &request_error)) {
			last_error = request_error;
			continue;
		}
		if (response.status_code < 200 || response.status_code >= 300) {
			last_error = firebase_error_message_from_body(response.body);
			continue;
		}
		return true;
	}

	if (error != nullptr) {
		*error = last_error.empty() ? "Unable to upload to Firebase Storage." : last_error;
	}
	return false;
}

bool delete_storage_object(const FirebaseAuthConfig &config,
                           FirebaseAuthSession *session,
                           const std::string &object_path,
                           bool *deleted_anything,
                           std::string *error)
{
	if (deleted_anything != nullptr) {
		*deleted_anything = false;
	}

	const std::vector<std::string> bucket_candidates = storage_bucket_candidates(config);
	const std::string encoded_object = url_encode(object_path);
	std::string last_error;
	for (const std::string &bucket : bucket_candidates) {
		const std::string url =
			std::string(kFirebaseStorageEndpoint) + "/v0/b/" + bucket + "/o/" + encoded_object;
		HttpResponse response;
		std::string request_error;
		if (!authorized_storage_request(config,
		                                session,
		                                "DELETE",
		                                url,
		                                "",
		                                "",
		                                &response,
		                                &request_error)) {
			last_error = request_error;
			continue;
		}
		if (response.status_code == 404) {
			return true;
		}
		if (response.status_code < 200 || response.status_code >= 300) {
			last_error = firebase_error_message_from_body(response.body);
			continue;
		}
		if (deleted_anything != nullptr) {
			*deleted_anything = true;
		}
		return true;
	}

	if (error != nullptr) {
		*error = last_error.empty() ? "Unable to delete the Firebase Storage object." : last_error;
	}
	return false;
}

}  // namespace

bool FirebaseCloudGrammarStorageService::listPrivateGrammars(
	const FirebaseAuthConfig &config,
	FirebaseAuthSession *session,
	std::vector<FirebasePrivateGrammarRecord> *grammars,
	std::string *error)
{
	if (grammars == nullptr) {
		if (error != nullptr) {
			*error = "Grammar list output was null.";
		}
		return false;
	}
	if (!prepare_storage_session(config, session, error)) {
		return false;
	}

	std::vector<FirebasePrivateGrammarRecord> private_items;
	if (!list_storage_prefix(config,
	                         session,
	                         storage_user_prefix(*session, kPrivateGrammarPrefix),
	                         &private_items,
	                         error)) {
		return false;
	}

	std::vector<FirebasePrivateGrammarRecord> published_items;
	std::string published_error;
	list_storage_prefix(config,
	                    session,
	                    storage_user_prefix(*session, kGalleryGrammarPrefix),
	                    &published_items,
	                    &published_error);

	std::unordered_map<std::string, bool> published_names;
	for (const FirebasePrivateGrammarRecord &item : published_items) {
		if (item.name.size() >= 8 && item.name.substr(item.name.size() - 8) == ".grammar") {
			published_names[item.name] = true;
		}
	}

	for (FirebasePrivateGrammarRecord &item : private_items) {
		item.published = published_names.find(item.name) != published_names.end();
	}

	std::sort(private_items.begin(),
	          private_items.end(),
	          [](const FirebasePrivateGrammarRecord &lhs,
	             const FirebasePrivateGrammarRecord &rhs) {
		          return lhs.name < rhs.name;
	          });

	*grammars = std::move(private_items);
	return true;
}

bool FirebaseCloudGrammarStorageService::loadPrivateGrammar(const FirebaseAuthConfig &config,
                                                            FirebaseAuthSession *session,
                                                            const std::string &grammar_name,
                                                            std::string *contents,
                                                            bool *published,
                                                            std::string *error)
{
	if (published != nullptr) {
		*published = false;
	}
	if (contents == nullptr) {
		if (error != nullptr) {
			*error = "Grammar contents output was null.";
		}
		return false;
	}
	if (!prepare_storage_session(config, session, error)) {
		return false;
	}

	const std::string sanitized_name = sanitize_grammar_name(grammar_name);
	if (!valid_grammar_name(sanitized_name)) {
		if (error != nullptr) {
			*error = "Grammar name is invalid.";
		}
		return false;
	}

	if (!download_storage_text(config,
	                           session,
	                           private_grammar_object_path(*session, sanitized_name),
	                           contents,
	                           error)) {
		return false;
	}

	if (published != nullptr) {
		std::vector<FirebasePrivateGrammarRecord> private_items;
		std::string list_error;
		if (listPrivateGrammars(config, session, &private_items, &list_error)) {
			for (const FirebasePrivateGrammarRecord &item : private_items) {
				if (item.name == sanitized_name) {
					*published = item.published;
					break;
				}
			}
		}
	}
	return true;
}

bool FirebaseCloudGrammarStorageService::savePrivateGrammar(const FirebaseAuthConfig &config,
                                                            FirebaseAuthSession *session,
                                                            const std::string &grammar_name,
                                                            const std::string &contents,
                                                            std::string *error)
{
	if (!prepare_storage_session(config, session, error)) {
		return false;
	}

	const std::string sanitized_name = sanitize_grammar_name(grammar_name);
	if (!valid_grammar_name(sanitized_name)) {
		if (error != nullptr) {
			*error = "Grammar name is invalid.";
		}
		return false;
	}
	if (contents.size() > kFirebaseGrammarMaxBytes) {
		if (error != nullptr) {
			*error = "Grammar exceeds the 30,000 byte Firebase Storage limit.";
		}
		return false;
	}

	return upload_storage_text(config,
	                           session,
	                           private_grammar_object_path(*session, sanitized_name),
	                           contents,
	                           "text/plain; charset=utf-8",
	                           error);
}

bool FirebaseCloudGrammarStorageService::publishGrammar(const FirebaseAuthConfig &config,
                                                        FirebaseAuthSession *session,
                                                        const std::string &grammar_name,
                                                        const std::string &contents,
                                                        const std::string &author_email,
                                                        std::string *error)
{
	if (!prepare_storage_session(config, session, error)) {
		return false;
	}

	const std::string sanitized_name = sanitize_grammar_name(grammar_name);
	if (!valid_grammar_name(sanitized_name)) {
		if (error != nullptr) {
			*error = "Grammar name is invalid.";
		}
		return false;
	}
	if (contents.size() > kFirebaseGrammarMaxBytes) {
		if (error != nullptr) {
			*error = "Grammar exceeds the 30,000 byte Firebase Storage limit.";
		}
		return false;
	}

	if (!savePrivateGrammar(config, session, sanitized_name, contents, error)) {
		return false;
	}

	if (!upload_storage_text(config,
	                         session,
	                         published_grammar_object_path(*session, sanitized_name),
	                         contents,
	                         "text/plain; charset=utf-8",
	                         error)) {
		return false;
	}

	const std::string published_at = current_iso8601_timestamp();
	const std::string title =
		sanitized_name.size() > 8 ? sanitized_name.substr(0, sanitized_name.size() - 8) : sanitized_name;
	std::ostringstream metadata;
	metadata << "{\n"
	         << "  \"title\": \"" << json_escape(title) << "\",\n"
	         << "  \"fileName\": \"" << json_escape(sanitized_name) << "\",\n"
	         << "  \"authorUid\": \"" << json_escape(session->local_id) << "\",\n"
	         << "  \"authorEmail\": \"" << json_escape(author_email) << "\",\n"
	         << "  \"privateObjectPath\": \"" << json_escape(private_grammar_object_path(*session, sanitized_name)) << "\",\n"
	         << "  \"galleryObjectPath\": \"" << json_escape(published_grammar_object_path(*session, sanitized_name)) << "\",\n"
	         << "  \"publishedAt\": \"" << json_escape(published_at) << "\",\n"
	         << "  \"sizeBytes\": " << contents.size() << ",\n"
	         << "  \"source\": \"progen3d-desktop\"\n"
	         << "}\n";

	return upload_storage_text(config,
	                           session,
	                           published_metadata_object_path(*session, sanitized_name),
	                           metadata.str(),
	                           "application/json; charset=utf-8",
	                           error);
}

bool FirebaseCloudGrammarStorageService::depublishGrammar(const FirebaseAuthConfig &config,
                                                          FirebaseAuthSession *session,
                                                          const std::string &grammar_name,
                                                          std::string *error)
{
	if (!prepare_storage_session(config, session, error)) {
		return false;
	}

	const std::string sanitized_name = sanitize_grammar_name(grammar_name);
	if (!valid_grammar_name(sanitized_name)) {
		if (error != nullptr) {
			*error = "Grammar name is invalid.";
		}
		return false;
	}

	bool removed_grammar = false;
	if (!delete_storage_object(config,
	                           session,
	                           published_grammar_object_path(*session, sanitized_name),
	                           &removed_grammar,
	                           error)) {
		return false;
	}

	bool removed_metadata = false;
	if (!delete_storage_object(config,
	                           session,
	                           published_metadata_object_path(*session, sanitized_name),
	                           &removed_metadata,
	                           error)) {
		return false;
	}

	if (!removed_grammar && !removed_metadata && error != nullptr) {
		*error = "This grammar is not currently published.";
		return false;
	}

	return true;
}

FirebaseGrammarStorageService &firebase_grammar_storage_service()
{
	static FirebaseCloudGrammarStorageService storage_service;
	return storage_service;
}

bool firebase_storage_list_private_grammars(const FirebaseAuthConfig &config,
                                            FirebaseAuthSession *session,
                                            std::vector<FirebaseStoredGrammar> *grammars,
                                            std::string *error)
{
	return firebase_grammar_storage_service().listPrivateGrammars(config, session, grammars, error);
}

bool firebase_storage_load_private_grammar(const FirebaseAuthConfig &config,
                                           FirebaseAuthSession *session,
                                           const std::string &grammar_name,
                                           std::string *contents,
                                           bool *published,
                                           std::string *error)
{
	return firebase_grammar_storage_service().loadPrivateGrammar(config,
	                                                            session,
	                                                            grammar_name,
	                                                            contents,
	                                                            published,
	                                                            error);
}

bool firebase_storage_save_private_grammar(const FirebaseAuthConfig &config,
                                           FirebaseAuthSession *session,
                                           const std::string &grammar_name,
                                           const std::string &contents,
                                           std::string *error)
{
	return firebase_grammar_storage_service().savePrivateGrammar(config,
	                                                            session,
	                                                            grammar_name,
	                                                            contents,
	                                                            error);
}

bool firebase_storage_publish_grammar(const FirebaseAuthConfig &config,
                                      FirebaseAuthSession *session,
                                      const std::string &grammar_name,
                                      const std::string &contents,
                                      const std::string &author_email,
                                      std::string *error)
{
	return firebase_grammar_storage_service().publishGrammar(config,
	                                                        session,
	                                                        grammar_name,
	                                                        contents,
	                                                        author_email,
	                                                        error);
}

bool firebase_storage_depublish_grammar(const FirebaseAuthConfig &config,
                                        FirebaseAuthSession *session,
                                        const std::string &grammar_name,
                                        std::string *error)
{
	return firebase_grammar_storage_service().depublishGrammar(config,
	                                                          session,
	                                                          grammar_name,
	                                                          error);
}
