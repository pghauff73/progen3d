#include "editor/service/FirebaseAuthenticationService.h"

bool FirebaseAuthenticationService::signInWithPassword(
	const FirebaseAuthConfig &configuration,
	const std::string &email,
	const std::string &password,
	FirebaseAuthSession *session,
	std::string *error_message)
{
	return firebase_auth_sign_in(configuration, email, password, session, error_message);
}

bool FirebaseAuthenticationService::signInWithGoogle(
	const FirebaseAuthConfig &configuration,
	FirebaseAuthSession *session,
	std::string *error_message)
{
	return firebase_auth_sign_in_with_google(configuration, session, error_message);
}

bool FirebaseAuthenticationService::createPasswordAccount(
	const FirebaseAuthConfig &configuration,
	const std::string &email,
	const std::string &password,
	FirebaseAuthSession *session,
	std::string *error_message)
{
	return firebase_auth_sign_up(configuration, email, password, session, error_message);
}

bool FirebaseAuthenticationService::refreshSession(
	const FirebaseAuthConfig &configuration,
	FirebaseAuthSession *session,
	std::string *error_message)
{
	if (session == nullptr) {
		if (error_message != nullptr) {
			*error_message = "Firebase session refresh requires a session destination.";
		}
		return false;
	}
	return firebase_auth_refresh(configuration, session, error_message);
}
