#pragma once

#include "grammar/model/GeometryExpression.h"
#include "grammar/model/GrammarSourceRange.h"
#include "grammar/model/SpatialClearanceSyntax.h"
#include "grammar/service/SpatialDeclarationParserTypes.h"

#include <array>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

class SpatialDeclarationFieldReader
{
public:
	static bool isIdentifier(const std::string &text)
	{
		if (text.empty()) return false;
		const unsigned char first = static_cast<unsigned char>(text.front());
		if (std::isalpha(first) == 0 && text.front() != '_') return false;
		for (std::size_t index = 1; index < text.size(); ++index) {
			const unsigned char character = static_cast<unsigned char>(text[index]);
			if (std::isalnum(character) == 0 && text[index] != '_') return false;
		}
		return true;
	}

	static bool beginDeclaration(const std::vector<std::string> &raw_tokens,
	                             std::size_t *index,
	                             const std::string &declaration_name,
	                             std::string *diagnostic)
	{
		if (index == nullptr || *index >= raw_tokens.size() || raw_tokens[*index] != "(") {
			if (diagnostic != nullptr) {
				*diagnostic = declaration_name + " requires an opening parenthesis.";
			}
			return false;
		}
		++(*index);
		return true;
	}

	static bool beginField(const std::vector<std::string> &raw_tokens,
	                       std::size_t *index,
	                       std::string *field_name,
	                       std::size_t *field_start_index,
	                       std::string *diagnostic)
	{
		if (index == nullptr || field_name == nullptr || field_start_index == nullptr ||
		    *index >= raw_tokens.size() || !isIdentifier(raw_tokens[*index])) {
			if (diagnostic != nullptr) *diagnostic = "Spatial declaration requires a named field.";
			return false;
		}
		*field_start_index = *index;
		*field_name = raw_tokens[(*index)++];
		if (*index >= raw_tokens.size() || raw_tokens[*index] != "(") {
			if (diagnostic != nullptr) {
				*diagnostic = "Spatial field '" + *field_name + "' requires an opening parenthesis.";
			}
			return false;
		}
		++(*index);
		return true;
	}

	static bool endField(const std::vector<std::string> &raw_tokens,
	                     std::size_t *index,
	                     const std::string &field_name,
	                     std::string *diagnostic)
	{
		if (index == nullptr || *index >= raw_tokens.size() || raw_tokens[*index] != ")") {
			if (diagnostic != nullptr) {
				*diagnostic = "Spatial field '" + field_name + "' is missing its closing parenthesis.";
			}
			return false;
		}
		++(*index);
		return true;
	}

	static bool readIdentifier(const std::vector<std::string> &raw_tokens,
	                           std::size_t *index,
	                           const std::string &field_name,
	                           std::string *value,
	                           std::string *diagnostic)
	{
		if (index == nullptr || value == nullptr || *index >= raw_tokens.size() ||
		    !isIdentifier(raw_tokens[*index])) {
			if (diagnostic != nullptr) {
				*diagnostic = "Spatial field '" + field_name + "' requires an identifier.";
			}
			return false;
		}
		*value = raw_tokens[(*index)++];
		return true;
	}

	static bool readIdentifierList(const std::vector<std::string> &raw_tokens,
	                               std::size_t *index,
	                               const std::string &field_name,
	                               bool allow_empty,
	                               std::vector<std::string> *values,
	                               std::string *diagnostic)
	{
		if (index == nullptr || values == nullptr) return false;
		while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
			if (!isIdentifier(raw_tokens[*index])) {
				if (diagnostic != nullptr) {
					*diagnostic = "Spatial field '" + field_name + "' accepts identifiers only.";
				}
				return false;
			}
			values->push_back(raw_tokens[(*index)++]);
		}
		if (!allow_empty && values->empty()) {
			if (diagnostic != nullptr) {
				*diagnostic = "Spatial field '" + field_name + "' requires at least one identifier.";
			}
			return false;
		}
		return true;
	}

	static bool readExpression(
		const std::vector<std::string> &raw_tokens,
		std::size_t *index,
		const std::string &field_name,
		const SpatialDeclarationParserTypes::ExpressionArgumentParser &expression_parser,
		GeometryExpression *expression,
		std::string *diagnostic)
	{
		if (index == nullptr || expression == nullptr || !expression_parser) return false;
		try {
			*expression = GeometryExpression(expression_parser(raw_tokens, index, ")"));
			return true;
		}
		catch (...) {
			if (diagnostic != nullptr) {
				*diagnostic = "Could not parse expression in spatial field '" + field_name + "'.";
			}
			return false;
		}
	}

	static bool readVector3(
		const std::vector<std::string> &raw_tokens,
		std::size_t *index,
		const std::string &field_name,
		const SpatialDeclarationParserTypes::ExpressionArgumentParser &expression_parser,
		std::array<GeometryExpression, 3> *values,
		std::string *diagnostic)
	{
		if (values == nullptr) return false;
		for (GeometryExpression &value : *values) {
			if (!readExpression(raw_tokens, index, field_name, expression_parser, &value, diagnostic)) {
				return false;
			}
		}
		return true;
	}

	static bool readClearance(
		const std::vector<std::string> &raw_tokens,
		std::size_t *index,
		std::size_t field_start_index,
		const SpatialDeclarationParserTypes::ExpressionArgumentParser &expression_parser,
		const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
		SpatialClearanceSyntax *clearance,
		std::string *diagnostic)
	{
		GeometryExpression nominal("0");
		if (!readExpression(raw_tokens, index, "clearance", expression_parser, &nominal, diagnostic)) {
			return false;
		}
		GeometryExpression minimum(nominal.sourceText());
		GeometryExpression maximum(nominal.sourceText());
		if (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
			if (!readExpression(raw_tokens, index, "clearance", expression_parser, &minimum, diagnostic) ||
			    !readExpression(raw_tokens, index, "clearance", expression_parser, &maximum, diagnostic)) {
				return false;
			}
		}
		if (!endField(raw_tokens, index, "clearance", diagnostic)) return false;
		*clearance = SpatialClearanceSyntax(
			std::move(nominal),
			std::move(minimum),
			std::move(maximum),
			resolveRange(source_range_resolver, field_start_index, *index));
		return true;
	}

	static GrammarSourceRange resolveRange(
		const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
		std::size_t begin_index,
		std::size_t end_index)
	{
		return source_range_resolver
			? source_range_resolver(begin_index, end_index)
			: GrammarSourceRange();
	}
};
