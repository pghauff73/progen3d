#pragma once

#include "BackendApiClient.h"
#include "FirebaseAuth.h"

#include <array>
#include <string>

class AuthenticatedUserSession
{
public:
	FirebaseAuthConfig config;
	FirebaseAuthSession session;
	std::array<char, 192> api_key{};
	std::array<char, 128> project_id{};
	std::array<char, 192> google_client_id{};
	std::array<char, 192> google_client_secret{};
	std::array<char, 256> backend_base_url{};
	std::array<char, 160> email{};
	std::array<char, 160> password{};
	std::string status_message;
	bool status_is_error = false;
	bool show_password = false;
	BackendUserProfile backend_user;
};
