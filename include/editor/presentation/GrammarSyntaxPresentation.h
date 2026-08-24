#pragma once

#include "editor/model/GrammarEditorInteractionState.h"
#include "editor/model/GrammarSymbolIndex.h"

#include <functional>
#include <string>
#include <vector>

enum class GrammarSyntaxColorRole {
	TokenKind,
	NumericExpression,
	ConditionalExpression,
	ArithmeticExpression
};

class GrammarSyntaxPresentation
{
public:
	using PrimitiveRecognitionFunction = std::function<bool(const std::string &identifier)>;
	using TooltipResolutionFunction = std::function<std::string(const std::string &token)>;

	GrammarSyntaxPresentation(PrimitiveRecognitionFunction recognize_primitive,
	                          TooltipResolutionFunction resolve_operator_tooltip,
	                          TooltipResolutionFunction resolve_keyword_tooltip,
	                          TooltipResolutionFunction resolve_expression_tooltip,
	                          TooltipResolutionFunction resolve_primitive_tooltip);

	std::vector<EditorToken> tokenizeLine(const std::string &source_line,
	                                      const GrammarSymbolIndex &symbol_index) const;
	std::vector<EditorTokenSpan> tokenizeLineWithSpans(
		const std::string &source_line,
		const GrammarSymbolIndex &symbol_index) const;

	bool isSelectableIdentifier(EditorTokenKind token_kind) const;
	bool isMatchableIdentifier(EditorTokenKind token_kind) const;
	bool hasTooltip(const EditorToken &token) const;
	std::string labelForTokenKind(EditorTokenKind token_kind) const;
	GrammarSyntaxColorRole colorRoleForToken(const EditorToken &token) const;

private:
	PrimitiveRecognitionFunction recognize_primitive_;
	TooltipResolutionFunction resolve_operator_tooltip_;
	TooltipResolutionFunction resolve_keyword_tooltip_;
	TooltipResolutionFunction resolve_expression_tooltip_;
	TooltipResolutionFunction resolve_primitive_tooltip_;
};
