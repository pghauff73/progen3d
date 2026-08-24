#pragma once

#include "FirebaseAuth.h"

#include <string>

class AuthenticationService
{
public:
	virtual ~AuthenticationService() = default;

	virtual bool signInWithPassword(const FirebaseAuthConfig &configuration,
	                                const std::string &email,
	                                const std::string &password,
	                                FirebaseAuthSession *session,
	                                std::string *error_message) = 0;
	virtual bool signInWithGoogle(const FirebaseAuthConfig &configuration,
	                             FirebaseAuthSession *session,
	                             std::string *error_message) = 0;
	virtual bool createPasswordAccount(const FirebaseAuthConfig &configuration,
	                                   const std::string &email,
	                                   const std::string &password,
	                                   FirebaseAuthSession *session,
	                                   std::string *error_message) = 0;
	virtual bool refreshSession(const FirebaseAuthConfig &configuration,
	                            FirebaseAuthSession *session,
	                            std::string *error_message) = 0;
};
