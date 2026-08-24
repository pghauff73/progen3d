#include "editor/service/BackendCloudGrammarRepository.h"

#include <utility>

namespace {

class BackendCloudGrammarRepositoryRequest final : public CloudGrammarRepositoryRequest
{
public:
	explicit BackendCloudGrammarRepositoryRequest(BackendAsyncToken token)
		: token_(std::move(token))
	{
	}

	~BackendCloudGrammarRepositoryRequest() override
	{
		cancel();
	}

	void cancel() override
	{
		backend_api_cancel_async(token_);
		token_ = BackendAsyncToken{};
	}

private:
	BackendAsyncToken token_;
};

}

BackendCloudGrammarRepository::BackendCloudGrammarRepository(
	const FirebaseAuthConfig &configuration,
	FirebaseAuthSession &session)
	: configuration_(configuration),
	  session_(session)
{
}

bool BackendCloudGrammarRepository::listGrammars(
	std::vector<BackendFileSummary> *grammars,
	std::string *error_message)
{
	return backend_api_list_files(configuration_, &session_, grammars, error_message);
}

bool BackendCloudGrammarRepository::loadGrammar(
	const std::string &file_id,
	BackendFileRecord *grammar,
	std::string *error_message)
{
	return backend_api_get_file(configuration_, &session_, file_id, grammar, error_message);
}

bool BackendCloudGrammarRepository::loadPublicGrammar(
	const std::string &file_id,
	BackendFileRecord *grammar,
	std::string *error_message)
{
	return backend_api_get_public_file(configuration_, &session_, file_id, grammar, error_message);
}

bool BackendCloudGrammarRepository::saveGrammar(
	const std::string &file_id,
	const std::string &title,
	const std::string &source_text,
	BackendFileRecord *grammar,
	std::string *error_message)
{
	return backend_api_save_file(configuration_,
	                             &session_,
	                             file_id,
	                             title,
	                             source_text,
	                             grammar,
	                             error_message);
}

bool BackendCloudGrammarRepository::publishGrammar(
	const std::string &file_id,
	const std::string &title,
	const std::string &source_text,
	BackendFileRecord *grammar,
	std::string *error_message)
{
	return backend_api_publish_file(configuration_,
	                                &session_,
	                                file_id,
	                                title,
	                                source_text,
	                                grammar,
	                                error_message);
}

bool BackendCloudGrammarRepository::unpublishGrammar(
	const std::string &file_id,
	std::string *error_message)
{
	return backend_api_unpublish_file(configuration_, &session_, file_id, error_message);
}

std::shared_ptr<CloudGrammarRepositoryRequest>
BackendCloudGrammarRepository::requestGrammarList(CloudGrammarListCallback callback)
{
	BackendAsyncToken token = backend_api_list_files_async(
		configuration_,
		session_,
		[callback = std::move(callback)](
			const BackendAsyncResult<std::vector<BackendFileSummary>> &result) {
			if (callback) {
				callback(result);
			}
		});
	return std::make_shared<BackendCloudGrammarRepositoryRequest>(std::move(token));
}

std::shared_ptr<CloudGrammarRepositoryRequest>
BackendCloudGrammarRepository::requestGrammarLoad(
	const std::string &file_id,
	bool allow_public_fallback,
	CloudGrammarLoadCallback callback)
{
	const FirebaseAuthConfig configuration = configuration_;
	const FirebaseAuthSession session = session_;
	BackendAsyncToken token = backend_api_get_file_async(
		configuration,
		session,
		file_id,
		[configuration,
		 session,
		 file_id,
		 allow_public_fallback,
		 callback = std::move(callback)](const BackendAsyncResult<BackendFileRecord> &result) mutable {
			BackendAsyncResult<BackendFileRecord> final_result = result;
			if ((!final_result.success || final_result.value.id.empty()) && allow_public_fallback) {
				FirebaseAuthSession fallback_session = session;
				BackendFileRecord public_file;
				std::string public_error;
				if (backend_api_get_public_file(configuration,
				                                &fallback_session,
				                                file_id,
				                                &public_file,
				                                &public_error)) {
					final_result.success = true;
					final_result.error.clear();
					final_result.value = std::move(public_file);
				} else if (!public_error.empty()) {
					final_result.error = final_result.error.empty()
						? public_error
						: final_result.error + " Public fallback failed: " + public_error;
				}
			}
			if (callback) {
				callback(final_result);
			}
		});
	return std::make_shared<BackendCloudGrammarRepositoryRequest>(std::move(token));
}
