#include "editor/service/BackendAiGrammarProposalService.h"

#include <utility>

BackendAiGrammarProposalService::BackendAiGrammarProposalService(
	const FirebaseAuthConfig &configuration,
	FirebaseAuthSession &session)
	: configuration_(configuration),
	  session_(session)
{
}

bool BackendAiGrammarProposalService::listThreads(
	std::vector<BackendAiThreadSummary> *threads,
	std::string *error_message)
{
	return backend_api_list_ai_threads(configuration_, &session_, threads, error_message);
}

bool BackendAiGrammarProposalService::loadThread(
	const std::string &thread_id,
	BackendAiThreadSummary *thread,
	std::vector<BackendAiMessage> *messages,
	std::string *error_message)
{
	return backend_api_get_ai_thread(configuration_,
	                                 &session_,
	                                 thread_id,
	                                 thread,
	                                 messages,
	                                 error_message);
}

bool BackendAiGrammarProposalService::submitProposal(
	const BackendAiGenerateRequest &request,
	AiGrammarProposal *proposal,
	std::string *error_message)
{
	if (proposal == nullptr) {
		if (error_message != nullptr) {
			*error_message = "AI proposal output is required.";
		}
		return false;
	}

	BackendAiGenerateResponse response;
	const bool request_succeeded =
		backend_api_generate_ai(configuration_, &session_, request, &response, error_message);
	proposal->mode = response.mode;
	proposal->model_name = response.model;
	proposal->thread = std::move(response.thread);
	proposal->credits = std::move(response.credits);
	proposal->usage = std::move(response.usage);
	proposal->conversation_messages = std::move(response.messages);
	proposal->result = std::move(response.result);
	return request_succeeded;
}
