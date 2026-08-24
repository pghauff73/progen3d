#include "editor/model/AiGrammarProposalReview.h"

#include <algorithm>
#include <utility>

namespace {

class SourceLineCollection
{
public:
	std::vector<std::string> lines;
	bool ends_with_newline = false;
};

enum class DifferenceOperationKind {
	Equal,
	Remove,
	Add
};

class DifferenceOperation
{
public:
	DifferenceOperationKind kind = DifferenceOperationKind::Equal;
	std::string line;
};

SourceLineCollection split_source_lines(const std::string &source)
{
	SourceLineCollection collection;
	collection.ends_with_newline = !source.empty() && source.back() == '\n';
	std::size_t line_start = 0;
	while (line_start < source.size()) {
		const std::size_t line_end = source.find('\n', line_start);
		if (line_end == std::string::npos) {
			collection.lines.push_back(source.substr(line_start));
			break;
		}
		std::size_t content_end = line_end;
		if (content_end > line_start && source[content_end - 1] == '\r') {
			--content_end;
		}
		collection.lines.push_back(source.substr(line_start, content_end - line_start));
		line_start = line_end + 1;
	}
	if (source.empty()) {
		collection.lines.clear();
	}
	return collection;
}

std::string join_source_lines(const std::vector<std::string> &lines, bool ends_with_newline)
{
	std::string source;
	for (std::size_t line_index = 0; line_index < lines.size(); ++line_index) {
		if (line_index > 0) {
			source.push_back('\n');
		}
		source += lines[line_index];
	}
	if (ends_with_newline && !lines.empty()) {
		source.push_back('\n');
	}
	return source;
}

std::vector<DifferenceOperation> build_middle_operations(
	const std::vector<std::string> &original_lines,
	std::size_t original_begin,
	std::size_t original_end,
	const std::vector<std::string> &proposed_lines,
	std::size_t proposed_begin,
	std::size_t proposed_end)
{
	const std::size_t original_count = original_end - original_begin;
	const std::size_t proposed_count = proposed_end - proposed_begin;
	constexpr std::size_t kMaximumLcsCells = 2'000'000;
	if (original_count != 0 &&
	    proposed_count > kMaximumLcsCells / original_count) {
		std::vector<DifferenceOperation> operations;
		for (std::size_t index = original_begin; index < original_end; ++index) {
			operations.push_back({DifferenceOperationKind::Remove, original_lines[index]});
		}
		for (std::size_t index = proposed_begin; index < proposed_end; ++index) {
			operations.push_back({DifferenceOperationKind::Add, proposed_lines[index]});
		}
		return operations;
	}

	const std::size_t column_count = proposed_count + 1;
	std::vector<std::size_t> lcs_lengths((original_count + 1) * column_count, 0);
	const auto cell = [&](std::size_t original_index, std::size_t proposed_index) -> std::size_t & {
		return lcs_lengths[original_index * column_count + proposed_index];
	};
	for (std::size_t original_offset = original_count; original_offset-- > 0;) {
		for (std::size_t proposed_offset = proposed_count; proposed_offset-- > 0;) {
			if (original_lines[original_begin + original_offset] ==
			    proposed_lines[proposed_begin + proposed_offset]) {
				cell(original_offset, proposed_offset) = cell(original_offset + 1, proposed_offset + 1) + 1;
			} else {
				cell(original_offset, proposed_offset) =
					std::max(cell(original_offset + 1, proposed_offset),
					         cell(original_offset, proposed_offset + 1));
			}
		}
	}

	std::vector<DifferenceOperation> operations;
	std::size_t original_offset = 0;
	std::size_t proposed_offset = 0;
	while (original_offset < original_count || proposed_offset < proposed_count) {
		if (original_offset < original_count && proposed_offset < proposed_count &&
		    original_lines[original_begin + original_offset] ==
		        proposed_lines[proposed_begin + proposed_offset]) {
			operations.push_back(
				{DifferenceOperationKind::Equal, original_lines[original_begin + original_offset]});
			++original_offset;
			++proposed_offset;
		} else if (proposed_offset < proposed_count &&
		           (original_offset == original_count ||
		            cell(original_offset, proposed_offset + 1) >=
		                cell(original_offset + 1, proposed_offset))) {
			operations.push_back(
				{DifferenceOperationKind::Add, proposed_lines[proposed_begin + proposed_offset]});
			++proposed_offset;
		} else {
			operations.push_back(
				{DifferenceOperationKind::Remove, original_lines[original_begin + original_offset]});
			++original_offset;
		}
	}
	return operations;
}

std::vector<AiGrammarSourceDifference> calculate_source_differences(
	const std::vector<std::string> &original_lines,
	const std::vector<std::string> &proposed_lines)
{
	std::size_t matching_prefix_count = 0;
	while (matching_prefix_count < original_lines.size() &&
	       matching_prefix_count < proposed_lines.size() &&
	       original_lines[matching_prefix_count] == proposed_lines[matching_prefix_count]) {
		++matching_prefix_count;
	}

	std::size_t matching_suffix_count = 0;
	while (matching_suffix_count < original_lines.size() - matching_prefix_count &&
	       matching_suffix_count < proposed_lines.size() - matching_prefix_count &&
	       original_lines[original_lines.size() - matching_suffix_count - 1] ==
	           proposed_lines[proposed_lines.size() - matching_suffix_count - 1]) {
		++matching_suffix_count;
	}

	const std::vector<DifferenceOperation> operations = build_middle_operations(
		original_lines,
		matching_prefix_count,
		original_lines.size() - matching_suffix_count,
		proposed_lines,
		matching_prefix_count,
		proposed_lines.size() - matching_suffix_count);

	std::vector<AiGrammarSourceDifference> differences;
	std::size_t original_line_index = matching_prefix_count;
	std::optional<AiGrammarSourceDifference> current_difference;
	auto finish_difference = [&]() {
		if (current_difference.has_value()) {
			differences.push_back(std::move(*current_difference));
			current_difference.reset();
		}
	};
	for (const DifferenceOperation &operation : operations) {
		if (operation.kind == DifferenceOperationKind::Equal) {
			finish_difference();
			++original_line_index;
			continue;
		}
		if (!current_difference.has_value()) {
			current_difference = AiGrammarSourceDifference{};
			current_difference->original_start_line = original_line_index + 1;
		}
		if (operation.kind == DifferenceOperationKind::Remove) {
			current_difference->original_lines.push_back(operation.line);
			++original_line_index;
		} else {
			current_difference->proposed_lines.push_back(operation.line);
		}
	}
	finish_difference();
	return differences;
}

}

void AiGrammarProposalReview::beginReview(const std::string &original_source,
	                                      const std::string &proposed_source,
	                                      std::vector<std::string> warnings)
{
	original_source_ = original_source;
	proposed_source_ = proposed_source;
	warnings_ = std::move(warnings);
	const SourceLineCollection original = split_source_lines(original_source_);
	const SourceLineCollection proposed = split_source_lines(proposed_source_);
	differences_ = calculate_source_differences(original.lines, proposed.lines);
	decision_ = proposed_source_.empty()
		? AiGrammarProposalDecision::NoProposal
		: AiGrammarProposalDecision::PendingReview;
}

void AiGrammarProposalReview::clear()
{
	original_source_.clear();
	proposed_source_.clear();
	warnings_.clear();
	differences_.clear();
	decision_ = AiGrammarProposalDecision::NoProposal;
}

bool AiGrammarProposalReview::hasProposal() const
{
	return decision_ != AiGrammarProposalDecision::NoProposal;
}

bool AiGrammarProposalReview::isPending() const
{
	return decision_ == AiGrammarProposalDecision::PendingReview;
}

bool AiGrammarProposalReview::isBasedOnSource(const std::string &current_source) const
{
	return hasProposal() && current_source == original_source_;
}

AiGrammarProposalDecision AiGrammarProposalReview::decision() const
{
	return decision_;
}

const std::string &AiGrammarProposalReview::originalSource() const
{
	return original_source_;
}

const std::string &AiGrammarProposalReview::proposedSource() const
{
	return proposed_source_;
}

const std::vector<std::string> &AiGrammarProposalReview::warnings() const
{
	return warnings_;
}

const std::vector<AiGrammarSourceDifference> &AiGrammarProposalReview::differences() const
{
	return differences_;
}

bool AiGrammarProposalReview::setDifferenceSelected(std::size_t difference_index, bool selected)
{
	if (!isPending() || difference_index >= differences_.size()) {
		return false;
	}
	differences_[difference_index].selected = selected;
	return true;
}

std::optional<std::string> AiGrammarProposalReview::acceptFullProposal()
{
	if (!isPending()) {
		return std::nullopt;
	}
	decision_ = AiGrammarProposalDecision::AcceptedFullProposal;
	return proposed_source_;
}

std::optional<std::string> AiGrammarProposalReview::acceptSelectedChanges()
{
	if (!isPending()) {
		return std::nullopt;
	}
	const SourceLineCollection original = split_source_lines(original_source_);
	const SourceLineCollection proposed = split_source_lines(proposed_source_);
	std::vector<std::string> accepted_lines;
	std::size_t original_line_index = 0;
	bool accepted_any_difference = false;
	for (const AiGrammarSourceDifference &difference : differences_) {
		const std::size_t difference_start = difference.original_start_line - 1;
		while (original_line_index < difference_start && original_line_index < original.lines.size()) {
			accepted_lines.push_back(original.lines[original_line_index]);
			++original_line_index;
		}
		if (difference.selected) {
			accepted_lines.insert(accepted_lines.end(),
			                      difference.proposed_lines.begin(),
			                      difference.proposed_lines.end());
			accepted_any_difference = true;
		} else {
			accepted_lines.insert(accepted_lines.end(),
			                      difference.original_lines.begin(),
			                      difference.original_lines.end());
		}
		original_line_index += difference.original_lines.size();
	}
	accepted_lines.insert(accepted_lines.end(),
	                      original.lines.begin() + static_cast<std::ptrdiff_t>(original_line_index),
	                      original.lines.end());
	decision_ = AiGrammarProposalDecision::AcceptedSelectedChanges;
	return join_source_lines(accepted_lines,
	                         accepted_any_difference
	                             ? proposed.ends_with_newline
	                             : original.ends_with_newline);
}

void AiGrammarProposalReview::rejectProposal()
{
	if (isPending()) {
		decision_ = AiGrammarProposalDecision::Rejected;
	}
}
