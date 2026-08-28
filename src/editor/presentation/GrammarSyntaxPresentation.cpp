#include "editor/presentation/GrammarSyntaxPresentation.h"

#include <cctype>
#include <regex>
#include <unordered_set>
#include <utility>

namespace {

bool is_identifier_start(char character)
{
	const unsigned char value = static_cast<unsigned char>(character);
	return std::isalpha(value) != 0 || character == '_';
}

bool is_identifier_character(char character)
{
	const unsigned char value = static_cast<unsigned char>(character);
	return std::isalnum(value) != 0 || character == '_' || character == '.';
}

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

bool is_number_start(const std::string &source_text, std::size_t index)
{
	if (index >= source_text.size()) {
		return false;
	}
	const char current = source_text[index];
	if (std::isdigit(static_cast<unsigned char>(current)) != 0) {
		return true;
	}
	if (current == '.' && index + 1 < source_text.size() &&
	    std::isdigit(static_cast<unsigned char>(source_text[index + 1])) != 0) {
		return true;
	}
	if ((current == '-' || current == '+') && index + 1 < source_text.size()) {
		const char next = source_text[index + 1];
		if (std::isdigit(static_cast<unsigned char>(next)) != 0 || next == '.') {
			if (index == 0) {
				return true;
			}
			const char previous = source_text[index - 1];
			return std::isspace(static_cast<unsigned char>(previous)) != 0 ||
			       previous == '(' || previous == '[' || previous == '{' ||
			       previous == ',' || previous == ';';
		}
	}
	return false;
}

std::size_t consume_number(const std::string &source_text, std::size_t index)
{
	if (index < source_text.size() &&
	    (source_text[index] == '-' || source_text[index] == '+')) {
		++index;
	}
	bool seen_decimal_point = false;
	while (index < source_text.size()) {
		const char character = source_text[index];
		if (std::isdigit(static_cast<unsigned char>(character)) != 0) {
			++index;
			continue;
		}
		if (character == '.' && !seen_decimal_point) {
			seen_decimal_point = true;
			++index;
			continue;
		}
		break;
	}
	return index;
}

bool is_expression_operator(const std::string &token)
{
	return token == "+" || token == "-" || token == "*" || token == "/" ||
	       token == "^" || token == "&";
}

bool is_comparison_operator(const std::string &token)
{
	return token == "<" || token == ">" || token == "<=" || token == ">=" ||
	       token == "==" || token == "!=";
}

bool is_conditional_operator(const std::string &token)
{
	return token == "?" || token == ":";
}

}

GrammarSyntaxPresentation::GrammarSyntaxPresentation(
	PrimitiveRecognitionFunction recognize_primitive,
	TooltipResolutionFunction resolve_operator_tooltip,
	TooltipResolutionFunction resolve_keyword_tooltip,
	TooltipResolutionFunction resolve_expression_tooltip,
	TooltipResolutionFunction resolve_primitive_tooltip)
	: recognize_primitive_(std::move(recognize_primitive)),
	  resolve_operator_tooltip_(std::move(resolve_operator_tooltip)),
	  resolve_keyword_tooltip_(std::move(resolve_keyword_tooltip)),
	  resolve_expression_tooltip_(std::move(resolve_expression_tooltip)),
	  resolve_primitive_tooltip_(std::move(resolve_primitive_tooltip))
{
}

std::vector<EditorToken> GrammarSyntaxPresentation::tokenizeLine(
	const std::string &source_line,
	const GrammarSymbolIndex &symbol_index) const
{
	static const std::unordered_set<std::string> keywords = {
		"R", "R*", "S", "T", "I", "!I", "V", "VR", "M", "P", "G", "A", "D",
		"DSX", "DSY", "DSZ", "DTX", "DTY", "DTZ",
		"Object", "Interface", "Connect", "Position",
		"id", "name", "class", "taxonomy", "container", "layer", "mask",
		"type", "origin", "normal", "tangent", "region", "tolerance", "clearance",
		"source", "target", "insertionDepth", "moving", "mode", "direction",
		"seatingDepth", "maxDistance", "priority"};
	static const std::unordered_set<std::string> expression_functions = {
		"sin", "cos", "tan", "abs", "sqrt", "floor", "ceil", "min", "max",
		"clamp", "wrap", "lerp", "cycle", "pingpong", "pulse", "accelerate",
		"decelerate", "ease", "smoother", "swing", "oscillate", "spin",
		"radians", "degrees"};

	std::vector<EditorToken> tokens;
	std::string definition_name;
	const std::regex rule_definition("^\\s*([A-Za-z_][A-Za-z0-9_]*)\\b.*->");
	std::smatch match;
	if (std::regex_search(source_line, match, rule_definition)) {
		definition_name = match[1].str();
	}

	const std::size_t comment_start = find_comment_start(source_line);
	const std::string source_code = source_line.substr(0, comment_start);
	std::size_t index = 0;
	while (index < source_code.size()) {
		const char character = source_code[index];
		if (std::isspace(static_cast<unsigned char>(character)) != 0) {
			const std::size_t start = index;
			while (index < source_code.size() &&
			       std::isspace(static_cast<unsigned char>(source_code[index])) != 0) {
				++index;
			}
			tokens.push_back({source_code.substr(start, index - start), EditorTokenKind::Plain, ""});
			continue;
		}

		if (source_code.compare(index, 2, "->") == 0) {
			tokens.push_back({"->", EditorTokenKind::Operator,
			                  resolve_operator_tooltip_ ? resolve_operator_tooltip_("->") : ""});
			index += 2;
			continue;
		}
		if (index + 1 < source_code.size()) {
			const std::string two_character_operator = source_code.substr(index, 2);
			if (two_character_operator == "<=" || two_character_operator == ">=" ||
			    two_character_operator == "==" || two_character_operator == "!=") {
				tokens.push_back({two_character_operator,
				                  EditorTokenKind::Operator,
				                  resolve_operator_tooltip_
					                  ? resolve_operator_tooltip_(two_character_operator)
					                  : ""});
				index += 2;
				continue;
			}
		}
		if (source_code.compare(index, 2, "R*") == 0 &&
		    (index + 2 >= source_code.size() || !is_identifier_character(source_code[index + 2]))) {
			tokens.push_back({"R*", EditorTokenKind::Keyword,
			                  resolve_keyword_tooltip_ ? resolve_keyword_tooltip_("R*") : ""});
			index += 2;
			continue;
		}
		if (source_code.compare(index, 2, "!I") == 0 &&
		    (index + 2 >= source_code.size() || !is_identifier_character(source_code[index + 2]))) {
			tokens.push_back({"!I", EditorTokenKind::Keyword,
			                  resolve_keyword_tooltip_ ? resolve_keyword_tooltip_("!I") : ""});
			index += 2;
			continue;
		}
		if (is_number_start(source_code, index)) {
			const std::size_t end = consume_number(source_code, index);
			tokens.push_back({source_code.substr(index, end - index),
			                  EditorTokenKind::Number,
			                  "Numeric literal used by a grammar expression or transform."});
			index = end;
			continue;
		}
		if (is_identifier_start(character)) {
			const std::size_t start = index;
			while (index < source_code.size() && is_identifier_character(source_code[index])) {
				++index;
			}
			const std::string identifier = source_code.substr(start, index - start);
			EditorTokenKind token_kind = EditorTokenKind::Identifier;
			std::string tooltip = "Identifier.";
			if (identifier == definition_name) {
				token_kind = EditorTokenKind::RuleDefinition;
				tooltip = "Rule definition. This line declares how this grammar symbol expands.";
			} else if (keywords.count(identifier) != 0U) {
				token_kind = EditorTokenKind::Keyword;
				tooltip = resolve_keyword_tooltip_ ? resolve_keyword_tooltip_(identifier) : "";
			} else if (recognize_primitive_ && recognize_primitive_(identifier)) {
				token_kind = EditorTokenKind::Primitive;
				tooltip = resolve_primitive_tooltip_ ? resolve_primitive_tooltip_(identifier) : "";
			} else if (identifier == "t") {
				token_kind = EditorTokenKind::Variable;
				tooltip = "Built-in immutable grammar time in seconds. P2 evaluates the scene as a deterministic snapshot at this t value.";
			} else if (expression_functions.count(identifier) != 0U) {
				tooltip = resolve_expression_tooltip_ ? resolve_expression_tooltip_(identifier) : "";
			} else if (symbol_index.containsVariableName(identifier)) {
				token_kind = EditorTokenKind::Variable;
				tooltip = "Grammar variable. Hovered symbol matches a declared runtime value.";
			} else if (symbol_index.containsRuleName(identifier)) {
				token_kind = EditorTokenKind::RuleReference;
				tooltip = "Rule reference. This symbol expands using another grammar production.";
			}
			tokens.push_back({identifier, token_kind, tooltip});
			continue;
		}
		if (std::string("|[]{}();,+-*/&?:<>=!^").find(character) != std::string::npos) {
			const std::string token(1, character);
			tokens.push_back({token,
			                  EditorTokenKind::Operator,
			                  resolve_operator_tooltip_ ? resolve_operator_tooltip_(token) : ""});
			++index;
			continue;
		}
		tokens.push_back({std::string(1, character),
		                  EditorTokenKind::Plain,
		                  "Unclassified symbol. The editor does not assign a specific grammar meaning to this character."});
		++index;
	}

	if (comment_start != std::string::npos) {
		tokens.push_back({source_line.substr(comment_start),
		                  EditorTokenKind::Comment,
		                  "Comment. Ignored by the grammar parser."});
	}
	if (tokens.empty()) {
		tokens.push_back({"", EditorTokenKind::Plain, ""});
	}
	return tokens;
}

std::vector<EditorTokenSpan> GrammarSyntaxPresentation::tokenizeLineWithSpans(
	const std::string &source_line,
	const GrammarSymbolIndex &symbol_index) const
{
	std::vector<EditorTokenSpan> spans;
	const std::vector<EditorToken> tokens = tokenizeLine(source_line, symbol_index);
	spans.reserve(tokens.size());
	int column = 0;
	for (const EditorToken &token : tokens) {
		const int start_column = column;
		column += static_cast<int>(token.text.size());
		spans.push_back({token, start_column, column});
	}
	return spans;
}

bool GrammarSyntaxPresentation::isSelectableIdentifier(EditorTokenKind token_kind) const
{
	return token_kind == EditorTokenKind::Identifier ||
	       token_kind == EditorTokenKind::Variable ||
	       token_kind == EditorTokenKind::RuleDefinition ||
	       token_kind == EditorTokenKind::RuleReference ||
	       token_kind == EditorTokenKind::Primitive;
}

bool GrammarSyntaxPresentation::isMatchableIdentifier(EditorTokenKind token_kind) const
{
	return token_kind == EditorTokenKind::Identifier ||
	       token_kind == EditorTokenKind::Variable ||
	       token_kind == EditorTokenKind::RuleDefinition ||
	       token_kind == EditorTokenKind::RuleReference;
}

bool GrammarSyntaxPresentation::hasTooltip(const EditorToken &token) const
{
	if (token.text.empty() || token.tooltip.empty()) {
		return false;
	}
	if (token.kind != EditorTokenKind::Plain) {
		return true;
	}
	for (char character : token.text) {
		if (std::isspace(static_cast<unsigned char>(character)) == 0) {
			return true;
		}
	}
	return false;
}

std::string GrammarSyntaxPresentation::labelForTokenKind(EditorTokenKind token_kind) const
{
	switch (token_kind) {
	case EditorTokenKind::RuleDefinition: return "Rule Definition";
	case EditorTokenKind::RuleReference: return "Rule Reference";
	case EditorTokenKind::Variable: return "Variable";
	case EditorTokenKind::Primitive: return "Primitive";
	case EditorTokenKind::Identifier: return "Identifier";
	case EditorTokenKind::Keyword: return "Keyword";
	case EditorTokenKind::Operator: return "Operator";
	case EditorTokenKind::Number: return "Number";
	case EditorTokenKind::Comment: return "Comment";
	case EditorTokenKind::Plain:
	default: return "Text";
	}
}

GrammarSyntaxColorRole GrammarSyntaxPresentation::colorRoleForToken(
	const EditorToken &token) const
{
	if (token.kind == EditorTokenKind::Number) {
		return GrammarSyntaxColorRole::NumericExpression;
	}
	if (token.kind == EditorTokenKind::Operator &&
	    (is_conditional_operator(token.text) || is_comparison_operator(token.text) ||
	     token.text == "=")) {
		return GrammarSyntaxColorRole::ConditionalExpression;
	}
	if (token.kind == EditorTokenKind::Operator && is_expression_operator(token.text)) {
		return GrammarSyntaxColorRole::ArithmeticExpression;
	}
	return GrammarSyntaxColorRole::TokenKind;
}
