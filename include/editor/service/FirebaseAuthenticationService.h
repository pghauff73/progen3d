#pragma once

#include "editor/service/AuthenticationService.h"

class FirebaseAuthenticationService final : public AuthenticationService
{
public:
	bool signInWithPassword(const FirebaseAuthConfig &configuration,
	                        const std::string &email,
	                        const std::string &password,
	                        FirebaseAuthSession *session,
	                        std::string *error_message) override;
	bool signInWithGoogle(const FirebaseAuthConfig &configuration,
	                     FirebaseAuthSession *session,
	                     std::string *error_message) override;
	bool createPasswordAccount(const FirebaseAuthConfig &configuration,
	                           const std::string &email,
	                           const std::string &password,
	                           FirebaseAuthSession *session,
	                           std::string *error_message) override;
	bool refreshSession(const FirebaseAuthConfig &configuration,
	                    FirebaseAuthSession *session,
	                    std::string *error_message) override;
};
