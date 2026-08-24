#pragma once

#include "BackendApiClient.h"
#include "editor/model/AiGrammarProposal.h"

#include <string>
#include <vector>

class AiGrammarProposalService
{
public:
	virtual ~AiGrammarProposalService() = default;

	virtual bool listThreads(std::vector<BackendAiThreadSummary> *threads,
	                         std::string *error_message) = 0;
	virtual bool loadThread(const std::string &thread_id,
	                       BackendAiThreadSummary *thread,
	                       std::vector<BackendAiMessage> *messages,
	                       std::string *error_message) = 0;
	virtual bool submitProposal(const BackendAiGenerateRequest &request,
	                           AiGrammarProposal *proposal,
	                           std::string *error_message) = 0;
};
