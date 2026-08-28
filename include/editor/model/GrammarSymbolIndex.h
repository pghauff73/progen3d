#pragma once

#include <string>
#include <unordered_set>
#include <vector>

class GrammarSymbolIndex
{
public:
	static GrammarSymbolIndex fromSourceLines(const std::vector<std::string> &source_lines);

	bool containsRuleName(const std::string &identifier) const;
	bool containsVariableName(const std::string &identifier) const;

private:
	std::unordered_set<std::string> rule_names_;
	std::unordered_set<std::string> variable_names_;
};
