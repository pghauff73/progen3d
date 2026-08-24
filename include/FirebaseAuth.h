#pragma once

#include <string>

#include <curl/curl.h>

class FirebaseAuthState {
public:
	virtual ~FirebaseAuthState() = default;
};

class FirebaseAuthConfig : public FirebaseAuthState {
public:
	std::string api_key;
	std::string project_id;
	std::string auth_domain;
	std::string storage_bucket;
	std::string google_client_id;
	std::string google_client_secret;
	std::string backend_base_url;
	std::string loaded_from;

	bool valid() const
	{
		return !api_key.empty() && !project_id.empty();
	}
};

class FirebaseAuthSession : public FirebaseAuthState {
public:
	bool authenticated = false;
	std::string email;
	std::string local_id;
	std::string id_token;
	std::string refresh_token;
	int expires_in_seconds = 0;
};

bool initialize_firebase_auth_support(std::string *error = nullptr);
bool firebase_auth_configure_curl_tls(CURL *curl, std::string *error = nullptr);
std::string firebase_auth_build_curl_error_message(CURL *curl,
                                                   CURLcode result,
                                                   const char *error_buffer,
                                                   const std::string &url);
void shutdown_firebase_auth_support();

const char *firebase_auth_default_config_path();
const char *firebase_auth_default_session_path();

bool load_firebase_auth_config(FirebaseAuthConfig *config, std::string *error = nullptr);
bool save_firebase_auth_config(const FirebaseAuthConfig &config, std::string *error = nullptr);

bool load_firebase_auth_session(FirebaseAuthSession *session, std::string *error = nullptr);
bool save_firebase_auth_session(const FirebaseAuthSession &session, std::string *error = nullptr);
void clear_firebase_auth_session_file();
bool open_url_in_browser(const std::string &url, std::string *error = nullptr);

bool firebase_auth_sign_in(const FirebaseAuthConfig &config,
                           const std::string &email,
                           const std::string &password,
                           FirebaseAuthSession *session,
                           std::string *error = nullptr);
bool firebase_auth_sign_in_with_google(const FirebaseAuthConfig &config,
                                       FirebaseAuthSession *session,
                                       std::string *error = nullptr);
bool firebase_auth_sign_up(const FirebaseAuthConfig &config,
                           const std::string &email,
                           const std::string &password,
                           FirebaseAuthSession *session,
                           std::string *error = nullptr);
bool firebase_auth_refresh(const FirebaseAuthConfig &config,
                           FirebaseAuthSession *session,
                           std::string *error = nullptr);
