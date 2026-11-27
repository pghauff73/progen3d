#include "editor/service/GrammarStructureValidationService.h"

#include "grammar.h"
#include "grammar/service/AudioEventDeclarationParser.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>
#include <utility>
#include <vector>

void debugout(const std::string &message);
void errorout(std::string message);

namespace {

struct ValidationTokenSpan {
	std::string text;
	int line = -1;
	int start_column = 0;
	int end_column = 0;
};

struct ValidationRuleBlock {
	int start_line = -1;
	std::vector<ValidationTokenSpan> tokens;
};

bool starts_new_rule(std::string_view trimmed_line)
{
	if (trimmed_line.empty()) {
		return false;
	}
	if (trimmed_line.rfind("->", 0) == 0) {
		return false;
	}
	return trimmed_line.find("->") != std::string::npos;
}

int first_non_whitespace_column(const std::string &line)
{
	const std::size_t start = line.find_first_not_of(" \t\r\n");
	return start == std::string::npos ? 0 : static_cast<int>(start);
}

int line_code_end_column(const std::string &line)
{
	std::size_t code_end = GrammarStructureValidationService::findCommentStart(line);
	if (code_end == std::string::npos) {
		code_end = line.size();
	}
	while (code_end > 0 &&
	       std::isspace(static_cast<unsigned char>(line[code_end - 1])) != 0) {
		--code_end;
	}
	return static_cast<int>(code_end);
}

GrammarDiagnostic make_line_grammar_diagnostic(const std::vector<std::string> &lines,
                                               int line_number,
                                               const std::string &message)
{
	GrammarDiagnostic diagnostic;
	diagnostic.line = line_number;
	diagnostic.message = message;
	if (line_number >= 0 && line_number < static_cast<int>(lines.size())) {
		const std::string &line = lines[static_cast<std::size_t>(line_number)];
		diagnostic.start_column = first_non_whitespace_column(line);
		diagnostic.end_column =
			std::max(diagnostic.start_column + 1, line_code_end_column(line));
	}
	return diagnostic;
}

GrammarDiagnostic make_token_grammar_diagnostic(const ValidationTokenSpan &token,
                                                const std::string &message)
{
	GrammarDiagnostic diagnostic;
	diagnostic.line = token.line;
	diagnostic.start_column = token.start_column;
	diagnostic.end_column = std::max(token.start_column + 1, token.end_column);
	diagnostic.message = message;
	return diagnostic;
}

std::vector<ValidationTokenSpan> tokenize_validation_rule_block(const std::vector<std::string> &lines,
                                                                int start_line,
                                                                int end_line)
{
	std::vector<ValidationTokenSpan> tokens;
	if (start_line < 0 || end_line < start_line) {
		return tokens;
	}

	for (int line_number = start_line;
	     line_number <= end_line &&
	     line_number < static_cast<int>(lines.size());
	     ++line_number) {
		const std::string &line = lines[static_cast<std::size_t>(line_number)];
		if (AudioEventDeclarationParser::recognizes(line)) continue;
		const std::size_t code_end_raw = GrammarStructureValidationService::findCommentStart(line);
		const std::size_t code_end =
			code_end_raw == std::string::npos ? line.size() : code_end_raw;
		const std::size_t code_start = line.find_first_not_of(" \t\r\n");
		if (code_start == std::string::npos || code_start >= code_end) {
			continue;
		}

		std::string current_token;
		int current_start_column = -1;
		const auto flush_token = [&]() {
			if (current_token.empty()) {
				return;
			}
			tokens.push_back({current_token,
			                 line_number,
			                 current_start_column,
			                 current_start_column + static_cast<int>(current_token.size())});
			current_token.clear();
			current_start_column = -1;
		};

		for (std::size_t column = code_start; column < code_end; ++column) {
			const char c = line[column];
			if (std::isspace(static_cast<unsigned char>(c)) != 0) {
				flush_token();
				continue;
			}
			if (c == '-' && column + 1 < code_end && line[column + 1] == '>') {
				flush_token();
				tokens.push_back({"->",
				                 line_number,
				                 static_cast<int>(column),
				                 static_cast<int>(column) + 2});
				++column;
				continue;
			}
			if (c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
			    c == '|' || c == ';' || c == '?' || c == ':') {
				flush_token();
				tokens.push_back({std::string(1, c),
				                 line_number,
				                 static_cast<int>(column),
				                 static_cast<int>(column) + 1});
				continue;
			}
			if (current_token.empty()) {
				current_start_column = static_cast<int>(column);
			}
			current_token += c;
		}
		flush_token();
	}

	return tokens;
}

std::vector<ValidationRuleBlock> build_validation_rule_blocks(const std::vector<std::string> &lines)
{
	std::vector<ValidationRuleBlock> blocks;
	int current_rule_start_line = -1;

	const auto finalize_block = [&](int end_line) {
		if (current_rule_start_line < 0 || end_line < current_rule_start_line) {
			return;
		}
		ValidationRuleBlock block;
		block.start_line = current_rule_start_line;
		block.tokens = tokenize_validation_rule_block(lines, current_rule_start_line, end_line);
		if (!block.tokens.empty()) {
			blocks.push_back(std::move(block));
		}
		current_rule_start_line = -1;
	};

	for (int line_number = 0; line_number < static_cast<int>(lines.size()); ++line_number) {
		const std::string &line = lines[static_cast<std::size_t>(line_number)];
		if (AudioEventDeclarationParser::recognizes(line)) continue;
		const std::size_t code_end_raw = GrammarStructureValidationService::findCommentStart(line);
		const std::size_t code_end =
			code_end_raw == std::string::npos ? line.size() : code_end_raw;
		const std::size_t code_start = line.find_first_not_of(" \t\r\n");
		if (code_start == std::string::npos || code_start >= code_end) {
			continue;
		}
		if (current_rule_start_line < 0) {
			current_rule_start_line = line_number;
			continue;
		}
		if (starts_new_rule(std::string_view(line).substr(code_start, code_end - code_start))) {
			finalize_block(line_number - 1);
			current_rule_start_line = line_number;
		}
	}

	finalize_block(static_cast<int>(lines.size()) - 1);
	return blocks;
}

}

std::size_t GrammarStructureValidationService::findCommentStart(const std::string &line)
{
	const std::size_t hash_comment = line.find('#');
	const std::size_t slash_comment = line.find("//");
	if (hash_comment == std::string::npos) {
		return slash_comment;
	}
	if (slash_comment == std::string::npos) {
		return hash_comment;
	}
	return std::min(hash_comment, slash_comment);
}

bool GrammarStructureValidationService::validate(
	GrammarDocument *gram, bool quiet, const DiagnosticSink &diagnostic_sink)
{
	if (gram == nullptr) {
		if (!quiet) {
			errorout("Grammar validation error: grammar is null.");
		}
		return false;
	}

	if (!quiet) {
		debugout("Validating grammar structure");
	}
	bool valid = true;

	std::size_t issue_count = 0;

	const auto add_validation_issue = [&](const GrammarDiagnostic &diagnostic) {
		valid = false;
		++issue_count;
		if (diagnostic_sink) {
			diagnostic_sink(diagnostic);
		}

		if (diagnostic.line >= 0 &&
		    std::find(gram->error_lines.begin(), gram->error_lines.end(), diagnostic.line) ==
		        gram->error_lines.end()) {
			gram->error_lines.push_back(diagnostic.line);
		}
		const bool duplicate = std::any_of(
			gram->error_details.begin(),
			gram->error_details.end(),
			[&](const GrammarDiagnostic &existing) {
				return existing.line == diagnostic.line &&
				       existing.start_column == diagnostic.start_column &&
				       existing.end_column == diagnostic.end_column &&
				       existing.message == diagnostic.message;
			});
		if (!duplicate) {
			gram->error_details.push_back(diagnostic);
		}

		std::ostringstream ss;
		ss << "Grammar validation error";
		if (diagnostic.line >= 0) {
			ss << " [line " << (diagnostic.line + 1);
			if (diagnostic.start_column >= 0) {
				ss << ", column " << (diagnostic.start_column + 1);
			}
			ss << "]";
		}
		ss << ": " << diagnostic.message;
		if (!quiet) {
			errorout(ss.str());
		}
	};

	const auto is_open_delimiter = [](char c) {
		return c == '(' || c == '[' || c == '{';
	};

	const auto is_close_delimiter = [](char c) {
		return c == ')' || c == ']' || c == '}';
	};

	const auto matching_delimiter = [](char c) {
		switch (c) {
		case '(':
			return ')';
		case '[':
			return ']';
		case '{':
			return '}';
		case ')':
			return '(';
		case ']':
			return '[';
		case '}':
			return '{';
		default:
			return '\0';
		}
	};

	const auto delimiters_match = [&](char open, char close) {
		return matching_delimiter(open) == close;
	};

	const std::vector<std::string> &source_lines = gram->lines;
	if (source_lines.empty()) {
		GrammarDiagnostic diagnostic;
		diagnostic.message = "Document is empty.";
		add_validation_issue(diagnostic);
		return false;
	}

	if (gram->rule_list.empty()) {
		GrammarDiagnostic diagnostic;
		diagnostic.message = "No valid rules were parsed.";
		add_validation_issue(diagnostic);
	} else if (!quiet) {
		debugout("Parsed rules: " + std::to_string(gram->rule_list.size()));
	}

	int empty_rules = 0;
	for (const auto& rule : gram->rule_list) {
		if (rule->tokens.empty()) {
			empty_rules++;
			if (!quiet) {
				errorout("Grammar validation warning: rule '" + rule->rule_name + "' has no tokens in its main section.");
			}
		}
	}

	if (empty_rules > 0 && !quiet) {
		debugout("Rules with empty main sections: " + std::to_string(empty_rules));
	}

	struct DelimiterState {
		char delimiter = '\0';
		int line = -1;
		int column = 0;
	};

	std::vector<DelimiterState> delimiter_stack;
	for (int line_index = 0; line_index < static_cast<int>(source_lines.size()); ++line_index) {
		const std::string &line = source_lines[static_cast<std::size_t>(line_index)];
		if (AudioEventDeclarationParser::recognizes(line)) continue;
		const std::size_t comment_column = findCommentStart(line);
		const std::size_t code_end = comment_column == std::string::npos ? line.size() : comment_column;
		for (std::size_t column = 0; column < code_end; ++column) {
			const char c = line[column];
			if (c == '\\') {
				add_validation_issue(
					{line_index,
					 static_cast<int>(column),
					 static_cast<int>(column) + 1,
					 "Unexpected '\\' character in grammar source."});
				continue;
			}
			if (is_open_delimiter(c)) {
				delimiter_stack.push_back({c, line_index, static_cast<int>(column)});
				continue;
			}
			if (!is_close_delimiter(c)) {
				continue;
			}
			if (delimiter_stack.empty()) {
				add_validation_issue(
					{line_index,
					 static_cast<int>(column),
					 static_cast<int>(column) + 1,
					 std::string("Unmatched closing delimiter '") + c + "'."});
				continue;
			}
			const DelimiterState open = delimiter_stack.back();
			if (delimiters_match(open.delimiter, c)) {
				delimiter_stack.pop_back();
				continue;
			}
			add_validation_issue(
				{line_index,
				 static_cast<int>(column),
				 static_cast<int>(column) + 1,
				 std::string("Mismatched closing delimiter '") + c +
				     "'. Expected '" + matching_delimiter(open.delimiter) + "'."});
		}
	}

	for (const DelimiterState &open : delimiter_stack) {
		add_validation_issue(
			{open.line,
			 open.column,
			 open.column + 1,
			 std::string("Unclosed delimiter '") + open.delimiter + "'."});
	}

	const std::vector<ValidationRuleBlock> rule_blocks =
		build_validation_rule_blocks(source_lines);
	for (const ValidationRuleBlock &block : rule_blocks) {
		std::vector<std::size_t> arrow_indices;
		for (std::size_t token_index = 0; token_index < block.tokens.size(); ++token_index) {
			if (block.tokens[token_index].text == "->") {
				arrow_indices.push_back(token_index);
			}
		}
		if (arrow_indices.empty()) {
			add_validation_issue(
				make_line_grammar_diagnostic(source_lines,
				                             block.start_line,
				                             "Rule is missing a '->' production arrow."));
			continue;
		}
		if (arrow_indices[0] == 0) {
			add_validation_issue(
				make_token_grammar_diagnostic(block.tokens[arrow_indices[0]],
				                              "Production arrow needs a rule name on the left-hand side."));
		}
		if (arrow_indices.size() > 2) {
			add_validation_issue(
				make_token_grammar_diagnostic(block.tokens[arrow_indices[2]],
				                              "Rule has too many '->' clauses. Only one alternate production is supported."));
		}

		const std::size_t primary_body_end =
			arrow_indices.size() > 1 ? arrow_indices[1] : block.tokens.size();
		if (arrow_indices[0] + 1 >= primary_body_end) {
			add_validation_issue(
				make_token_grammar_diagnostic(block.tokens[arrow_indices[0]],
				                              "Rule production body is empty."));
		}

		if (arrow_indices.size() > 1 && arrow_indices[1] + 1 >= block.tokens.size()) {
			add_validation_issue(
				make_token_grammar_diagnostic(block.tokens[arrow_indices[1]],
				                              "Alternate production body is empty."));
		}

		std::size_t section_start = arrow_indices[0] + 1;
		std::size_t pipe_count = 0;
		for (std::size_t token_index = arrow_indices[0] + 1;
		     token_index < primary_body_end;
		     ++token_index) {
			if (block.tokens[token_index].text != "|") {
				continue;
			}
			++pipe_count;
			if (section_start == token_index) {
				add_validation_issue(
					make_token_grammar_diagnostic(block.tokens[token_index],
					                              "Rule has an empty section around '|'."));
			}
			if (pipe_count > 2) {
				add_validation_issue(
					make_token_grammar_diagnostic(block.tokens[token_index],
					                              "Rule has too many '|' sections. At most three are supported."));
			}
			section_start = token_index + 1;
		}
		if (section_start == primary_body_end &&
		    primary_body_end > arrow_indices[0] + 1 &&
		    block.tokens[primary_body_end - 1].text == "|") {
			add_validation_issue(
				make_token_grammar_diagnostic(block.tokens[primary_body_end - 1],
				                              "Rule has an empty section around '|'."));
		}
	}

	if (!quiet) {
		if (issue_count != 0) {
			debugout("Grammar validation found " +
			         std::to_string(issue_count) + " issue(s)");
		} else {
			debugout("Grammar validation completed without additional structural issues");
		}
	}

	return valid;
}
