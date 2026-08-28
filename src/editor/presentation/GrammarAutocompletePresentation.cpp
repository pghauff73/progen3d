#include "editor/presentation/GrammarAutocompletePresentation.h"

#include <algorithm>
#include <cctype>

namespace {

std::string lowercase_copy(const std::string &value)
{
	std::string lowered = value;
	std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char character) {
		return static_cast<char>(std::tolower(character));
	});
	return lowered;
}

}

bool GrammarAutocompletePresentation::startsWithIgnoringCase(
	const std::string &candidate,
	const std::string &prefix) const
{
	if (prefix.size() > candidate.size()) {
		return false;
	}
	for (std::size_t index = 0; index < prefix.size(); ++index) {
		if (std::tolower(static_cast<unsigned char>(candidate[index])) !=
		    std::tolower(static_cast<unsigned char>(prefix[index]))) {
			return false;
		}
	}
	return true;
}

bool GrammarAutocompletePresentation::isStlQualifiedPrefix(const std::string &prefix) const
{
	const std::string lowered_prefix = lowercase_copy(prefix);
	return lowered_prefix == "stl" || lowered_prefix == "stl." ||
	       lowered_prefix.rfind("stl.", 0) == 0;
}

StlAutocompleteQuery GrammarAutocompletePresentation::parseStlQuery(
	const std::string &prefix) const
{
	StlAutocompleteQuery query;
	if (!isStlQualifiedPrefix(prefix)) {
		return query;
	}

	query.active = true;
	const std::string lowered_prefix = lowercase_copy(prefix);
	if (lowered_prefix == "stl") {
		query.category_context = true;
		return query;
	}

	const std::string suffix = lowered_prefix == "stl." ? "" : prefix.substr(4);
	query.display_prefix = suffix;
	const std::size_t separator = suffix.find('.');
	if (separator == std::string::npos) {
		query.category_context = true;
		query.category_prefix = suffix;
		return query;
	}

	query.category = suffix.substr(0, separator);
	query.name_prefix = suffix.substr(separator + 1);
	return query;
}

std::string GrammarAutocompletePresentation::resolveUniqueStlCategory(
	const std::string &category_query,
	const std::vector<std::string> &category_names) const
{
	if (category_query.empty()) {
		return {};
	}
	const std::string lowered_query = lowercase_copy(category_query);
	std::string prefix_match;
	for (const std::string &category_name : category_names) {
		if (lowercase_copy(category_name) == lowered_query) {
			return category_name;
		}
		if (!startsWithIgnoringCase(category_name, category_query)) {
			continue;
		}
		if (!prefix_match.empty()) {
			return {};
		}
		prefix_match = category_name;
	}
	return prefix_match;
}

std::vector<std::string> GrammarAutocompletePresentation::findPartClassSuggestions(
	const std::string &prefix,
	const std::vector<std::string> &part_class_names) const
{
	std::vector<std::string> suggestions;
	for (const std::string &part_class_name : part_class_names) {
		if (prefix.empty() || startsWithIgnoringCase(part_class_name, prefix)) {
			suggestions.push_back(part_class_name);
		}
	}
	if (suggestions.size() == 1 &&
	    lowercase_copy(suggestions.front()) == lowercase_copy(prefix)) {
		return {};
	}
	return suggestions;
}

std::vector<std::string> GrammarAutocompletePresentation::findMaterialTokenSuggestions(
	const std::string &trailing_fragment,
	const std::vector<std::string> &material_tokens) const
{
	std::vector<std::string> suggestions;
	for (const std::string &material_token : material_tokens) {
		if (trailing_fragment.empty() ||
		    startsWithIgnoringCase(material_token, trailing_fragment)) {
			suggestions.push_back(material_token);
		}
	}
	std::sort(suggestions.begin(), suggestions.end());
	return suggestions;
}

std::vector<std::string> GrammarAutocompletePresentation::findShapeOptionSuggestions(
	const std::string &prefix,
	const std::vector<std::string> &option_completions) const
{
	std::vector<std::string> suggestions;
	for (const std::string &completion : option_completions) {
		const std::size_t open_parenthesis = completion.find('(');
		const std::string option_name = completion.substr(0, open_parenthesis);
		if (prefix.empty() || startsWithIgnoringCase(option_name, prefix)) {
			suggestions.push_back(completion);
		}
	}
	std::sort(suggestions.begin(), suggestions.end());
	return suggestions;
}

std::vector<std::string>
GrammarAutocompletePresentation::findSpatialDeclarationSuggestions(
	const std::string &prefix,
	const std::vector<std::string> &declaration_completions) const
{
	return findShapeOptionSuggestions(prefix, declaration_completions);
}
