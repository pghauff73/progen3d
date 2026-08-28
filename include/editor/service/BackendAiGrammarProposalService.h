#pragma once

#include "editor/service/AiGrammarProposalService.h"

class BackendAiGrammarProposalService final : public AiGrammarProposalService
{
public:
	BackendAiGrammarProposalService(const FirebaseAuthConfig &configuration,
	                                FirebaseAuthSession &session);

	bool listThreads(std::vector<BackendAiThreadSummary> *threads,
	                 std::string *error_message) override;
	bool loadThread(const std::string &thread_id,
	               BackendAiThreadSummary *thread,
	               std::vector<BackendAiMessage> *messages,
	               std::string *error_message) override;
	bool submitProposal(const BackendAiGenerateRequest &request,
	                   AiGrammarProposal *proposal,
	                   std::string *error_message) override;

private:
	FirebaseAuthConfig configuration_;
	FirebaseAuthSession &session_;
};
