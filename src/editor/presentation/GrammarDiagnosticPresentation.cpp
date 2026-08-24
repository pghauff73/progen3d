#include "editor/presentation/GrammarDiagnosticPresentation.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>

namespace {

class DelimiterSourcePosition
{
public:
	char delimiter = '\0';
	int line = -1;
	int column = 0;
};

std::size_t find_comment_start(const std::string &source_line)
{
	const std::size_t hash_comment = source_line.find('#');
	const std::size_t slash_comment = source_line.find("//");
	if (hash_comment == std::string::npos) {
		return slash_comment;
	}
	if (slash_comment == std::string::npos) {
		return hash_comment;
	}
	return std::min(hash_comment, slash_comment);
}

std::string trim_source_code(const std::string &source_line)
{
	const std::size_t comment_start = find_comment_start(source_line);
	const std::string source_code = comment_start == std::string::npos
		                                ? source_line
		                                : source_line.substr(0, comment_start);
	const std::size_t first_non_space = source_code.find_first_not_of(" \t\r\n");
	if (first_non_space == std::string::npos) {
		return {};
	}
	const std::size_t last_non_space = source_code.find_last_not_of(" \t\r\n");
	return source_code.substr(first_non_space, last_non_space - first_non_space + 1);
}

int first_non_whitespace_column(const std::string &source_line)
{
	const std::size_t first_non_space = source_line.find_first_not_of(" \t\r\n");
	return first_non_space == std::string::npos ? 0 : static_cast<int>(first_non_space);
}

bool starts_new_rule(const std::string &trimmed_source_line)
{
	return !trimmed_source_line.empty() &&
	       trimmed_source_line.rfind("->", 0) != 0 &&
	       trimmed_source_line.find("->") != std::string::npos;
}

bool is_opening_delimiter(char character)
{
	return character == '(' || character == '[' || character == '{';
}

bool is_closing_delimiter(char character)
{
	return character == ')' || character == ']' || character == '}';
}

char matching_delimiter(char character)
{
	switch (character) {
	case '(': return ')';
	case '[': return ']';
	case '{': return '}';
	case ')': return '(';
	case ']': return '[';
	case '}': return '{';
	default: return '\0';
	}
}

bool delimiters_match(char opening_delimiter, char closing_delimiter)
{
	return matching_delimiter(opening_delimiter) == closing_delimiter;
}

EditorDiagnostic make_diagnostic(int line,
	                              int start_column,
	                              int end_column,
	                              EditorDiagnosticSeverity severity,
	                              std::string message)
{
	EditorDiagnostic diagnostic;
	diagnostic.range.line = line;
	diagnostic.range.start_column = std::max(0, start_column);
	diagnostic.range.end_column = std::max(diagnostic.range.start_column + 1, end_column);
	diagnostic.range.found = line >= 0;
	diagnostic.severity = severity;
	diagnostic.message = std::move(message);
	return diagnostic;
}

}

std::vector<EditorDiagnostic> GrammarDiagnosticPresentation::buildDiagnostics(
	const std::vector<std::string> &source_lines,
	const DocumentDiagnosticCollection *compiler_diagnostics) const
{
	std::vector<EditorDiagnostic> diagnostics;
	std::vector<DelimiterSourcePosition> delimiter_stack;
	int rule_block_start_line = -1;
	bool rule_block_has_arrow = false;

	const auto finish_rule_block = [&]() {
		if (rule_block_start_line < 0 ||
		    rule_block_start_line >= static_cast<int>(source_lines.size())) {
			rule_block_start_line = -1;
			rule_block_has_arrow = false;
			return;
		}
		if (!rule_block_has_arrow) {
			const std::string &source_line =
				source_lines[static_cast<std::size_t>(rule_block_start_line)];
			const std::size_t comment_start = find_comment_start(source_line);
			const int start_column = first_non_whitespace_column(source_line);
			const int end_column = static_cast<int>(
				comment_start == std::string::npos ? source_line.size() : comment_start);
			diagnostics.push_back(make_diagnostic(
				rule_block_start_line,
				start_column,
				std::max(start_column + 1, end_column),
				EditorDiagnosticSeverity::Error,
				"Rule block is missing a '->' production arrow."));
		}
		rule_block_start_line = -1;
		rule_block_has_arrow = false;
	};

	for (int line_index = 0; line_index < static_cast<int>(source_lines.size()); ++line_index) {
		const std::string &source_line = source_lines[static_cast<std::size_t>(line_index)];
		const std::string trimmed_source_line = trim_source_code(source_line);
		if (!trimmed_source_line.empty()) {
			if (rule_block_start_line < 0) {
				rule_block_start_line = line_index;
				rule_block_has_arrow = trimmed_source_line.find("->") != std::string::npos;
				if (trimmed_source_line.rfind("->", 0) == 0) {
					const int start_column = first_non_whitespace_column(source_line);
					diagnostics.push_back(make_diagnostic(
						line_index,
						start_column,
						start_column + 2,
						EditorDiagnosticSeverity::Error,
						"Production arrow needs a rule name on the left-hand side."));
				}
			} else if (starts_new_rule(trimmed_source_line)) {
				finish_rule_block();
				rule_block_start_line = line_index;
				rule_block_has_arrow = true;
			} else if (trimmed_source_line.find("->") != std::string::npos) {
				rule_block_has_arrow = true;
			}
		}

		const std::size_t comment_start = find_comment_start(source_line);
		const std::size_t source_code_end =
			comment_start == std::string::npos ? source_line.size() : comment_start;
		for (std::size_t column = 0; column < source_code_end; ++column) {
			const char character = source_line[column];
			if (is_opening_delimiter(character)) {
				delimiter_stack.push_back(
					{character, line_index, static_cast<int>(column)});
				continue;
			}
			if (!is_closing_delimiter(character)) {
				continue;
			}
			if (delimiter_stack.empty()) {
				diagnostics.push_back(make_diagnostic(
					line_index,
					static_cast<int>(column),
					static_cast<int>(column) + 1,
					EditorDiagnosticSeverity::Error,
					std::string("Unmatched closing delimiter '") + character + "'."));
				continue;
			}
			const DelimiterSourcePosition &opening_position = delimiter_stack.back();
			if (delimiters_match(opening_position.delimiter, character)) {
				delimiter_stack.pop_back();
				continue;
			}
			diagnostics.push_back(make_diagnostic(
				line_index,
				static_cast<int>(column),
				static_cast<int>(column) + 1,
				EditorDiagnosticSeverity::Error,
				std::string("Mismatched closing delimiter '") + character +
					"'. Expected '" + matching_delimiter(opening_position.delimiter) + "'."));
		}
	}

	finish_rule_block();
	for (const DelimiterSourcePosition &opening_position : delimiter_stack) {
		diagnostics.push_back(make_diagnostic(
			opening_position.line,
			opening_position.column,
			opening_position.column + 1,
			EditorDiagnosticSeverity::Error,
			std::string("Unclosed delimiter '") + opening_position.delimiter + "'."));
	}

	if (compiler_diagnostics == nullptr) {
		return diagnostics;
	}
	for (const GrammarDiagnostic &compiler_diagnostic : compiler_diagnostics->diagnostics()) {
		if (compiler_diagnostic.line < 0 ||
		    compiler_diagnostic.line >= static_cast<int>(source_lines.size())) {
			continue;
		}
		const std::string &source_line =
			source_lines[static_cast<std::size_t>(compiler_diagnostic.line)];
		const int start_column =
			std::clamp(compiler_diagnostic.start_column, 0, static_cast<int>(source_line.size()));
		const int end_column = std::clamp(
			std::max(start_column + 1, compiler_diagnostic.end_column),
			start_column + 1,
			static_cast<int>(source_line.size()) + 1);

		bool merged_with_existing_diagnostic = false;
		for (EditorDiagnostic &existing_diagnostic : diagnostics) {
			if (existing_diagnostic.range.line == compiler_diagnostic.line &&
			    existing_diagnostic.range.start_column == start_column &&
			    existing_diagnostic.range.end_column == end_column) {
				if (compiler_diagnostic.message.size() > existing_diagnostic.message.size()) {
					existing_diagnostic.message = compiler_diagnostic.message;
				}
				merged_with_existing_diagnostic = true;
				break;
			}
		}
		if (!merged_with_existing_diagnostic) {
			diagnostics.push_back(make_diagnostic(
				compiler_diagnostic.line,
				start_column,
				end_column,
				EditorDiagnosticSeverity::Error,
				compiler_diagnostic.message));
		}
	}
	return diagnostics;
}
