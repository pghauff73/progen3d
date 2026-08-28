#pragma once

#include "BackendApiClient.h"

#include <string>
#include <vector>

class AiGrammarProposal
{
public:
	bool hasProposedGrammar() const
	{
		return result.loaded && !result.grammar.empty();
	}

	std::string mode;
	std::string model_name;
	BackendAiThreadSummary thread;
	BackendCreditSummary credits;
	BackendAiUsageSummary usage;
	std::vector<BackendAiMessage> conversation_messages;
	BackendAiResult result;
};
