#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

class AiGrammarSourceDifference
{
public:
	std::size_t original_start_line = 1;
	std::vector<std::string> original_lines;
	std::vector<std::string> proposed_lines;
	bool selected = true;
};

enum class AiGrammarProposalDecision {
	NoProposal,
	PendingReview,
	AcceptedFullProposal,
	AcceptedSelectedChanges,
	Rejected
};

class AiGrammarProposalReview
{
public:
	void beginReview(const std::string &original_source,
	                 const std::string &proposed_source,
	                 std::vector<std::string> warnings = {});
	void clear();

	bool hasProposal() const;
	bool isPending() const;
	bool isBasedOnSource(const std::string &current_source) const;
	AiGrammarProposalDecision decision() const;
	const std::string &originalSource() const;
	const std::string &proposedSource() const;
	const std::vector<std::string> &warnings() const;
	const std::vector<AiGrammarSourceDifference> &differences() const;

	bool setDifferenceSelected(std::size_t difference_index, bool selected);
	std::optional<std::string> acceptFullProposal();
	std::optional<std::string> acceptSelectedChanges();
	void rejectProposal();

private:
	std::string original_source_;
	std::string proposed_source_;
	std::vector<std::string> warnings_;
	std::vector<AiGrammarSourceDifference> differences_;
	AiGrammarProposalDecision decision_ = AiGrammarProposalDecision::NoProposal;
};
