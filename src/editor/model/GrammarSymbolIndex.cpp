#include "editor/model/GrammarSymbolIndex.h"

#include <algorithm>
#include <cctype>
#include <regex>

namespace {

std::string trim_source_code(const std::string &source_line)
{
	const std::size_t hash_comment = source_line.find('#');
	const std::size_t slash_comment = source_line.find("//");
	std::size_t comment_start = std::string::npos;
	if (hash_comment == std::string::npos) {
		comment_start = slash_comment;
	} else if (slash_comment == std::string::npos) {
		comment_start = hash_comment;
	} else {
		comment_start = std::min(hash_comment, slash_comment);
	}

	const std::string code = comment_start == std::string::npos
		                         ? source_line
		                         : source_line.substr(0, comment_start);
	const auto first_non_space = std::find_if_not(code.begin(), code.end(), [](unsigned char character) {
		return std::isspace(character) != 0;
	});
	if (first_non_space == code.end()) {
		return {};
	}
	const auto last_non_space = std::find_if_not(code.rbegin(), code.rend(), [](unsigned char character) {
		return std::isspace(character) != 0;
	}).base();
	return std::string(first_non_space, last_non_space);
}

}

GrammarSymbolIndex GrammarSymbolIndex::fromSourceLines(
	const std::vector<std::string> &source_lines)
{
	GrammarSymbolIndex symbol_index;
	symbol_index.variable_names_.insert("t");
	const std::regex rule_definition("^\\s*([A-Za-z_][A-Za-z0-9_]*)\\b.*->");
	const std::regex variable_declaration("\\bR\\*?\\s+([A-Za-z_][A-Za-z0-9_]*)\\b");

	for (const std::string &source_line : source_lines) {
		const std::string source_code = trim_source_code(source_line);
		if (source_code.empty()) {
			continue;
		}

		std::smatch match;
		if (std::regex_search(source_code, match, rule_definition)) {
			symbol_index.rule_names_.insert(match[1].str());
		}
		auto search_start = source_code.cbegin();
		while (std::regex_search(search_start,
		                         source_code.cend(),
		                         match,
		                         variable_declaration)) {
			symbol_index.variable_names_.insert(match[1].str());
			search_start = match.suffix().first;
		}
	}

	return symbol_index;
}

bool GrammarSymbolIndex::containsRuleName(const std::string &identifier) const
{
	return rule_names_.count(identifier) != 0U;
}

bool GrammarSymbolIndex::containsVariableName(const std::string &identifier) const
{
	return variable_names_.count(identifier) != 0U;
}
