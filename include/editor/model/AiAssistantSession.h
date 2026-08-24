#pragma once

#include "BackendApiClient.h"
#include "editor/model/AiGrammarProposalReview.h"

#include <string>
#include <vector>

class AiAssistantSession
{
public:
	std::string mode = "active_helper_chat";
	std::string request_input;
	std::vector<BackendAiThreadSummary> threads;
	std::vector<BackendAiMessage> messages;
	BackendAiThreadSummary current_thread;
	BackendAiUsageSummary last_usage;
	BackendAiResult last_result;
	AiGrammarProposalReview proposal_review;
	std::string active_model;
	std::string selected_thread_id;
	std::string status_message;
	bool status_is_error = false;
	bool threads_loaded = false;
};
