#pragma once

#include "editor/model/GrammarEditorInteractionState.h"

#include <string>
#include <vector>

class GrammarAutocompletePresentation
{
public:
	bool startsWithIgnoringCase(const std::string &candidate,
	                            const std::string &prefix) const;
	bool isStlQualifiedPrefix(const std::string &prefix) const;
	StlAutocompleteQuery parseStlQuery(const std::string &prefix) const;
	std::string resolveUniqueStlCategory(const std::string &category_query,
	                                     const std::vector<std::string> &category_names) const;
	std::vector<std::string> findPartClassSuggestions(
		const std::string &prefix,
		const std::vector<std::string> &part_class_names) const;
	std::vector<std::string> findMaterialTokenSuggestions(
		const std::string &trailing_fragment,
		const std::vector<std::string> &material_tokens) const;
	std::vector<std::string> findShapeOptionSuggestions(
		const std::string &prefix,
		const std::vector<std::string> &option_completions) const;
	std::vector<std::string> findSpatialDeclarationSuggestions(
		const std::string &prefix,
		const std::vector<std::string> &declaration_completions) const;
};
