#include "grammar/service/SpatialConnectionDeclarationParser.h"

#include "SpatialDeclarationFieldReader.h"

#include <set>
#include <utility>

std::shared_ptr<const SpatialConnectionDeclarationSyntax>
SpatialConnectionDeclarationParser::parse(
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	std::size_t declaration_start_index,
	const SpatialDeclarationParserTypes::ExpressionArgumentParser &expression_parser,
	const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
	std::string *diagnostic) const
{
	if (!SpatialDeclarationFieldReader::beginDeclaration(
			raw_tokens, index, "Connect", diagnostic)) {
		return {};
	}

	std::string connection_id;
	std::string source_object_id;
	std::string source_interface_id;
	std::string target_object_id;
	std::string target_interface_id;
	std::string connection_type;
	SpatialClearanceSyntax clearance;
	GeometryExpression insertion_depth("0");
	std::set<std::string> declared_fields;

	while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
		std::string field_name;
		std::size_t field_start_index = 0;
		if (!SpatialDeclarationFieldReader::beginField(
				raw_tokens, index, &field_name, &field_start_index, diagnostic)) {
			return {};
		}
		if (!declared_fields.insert(field_name).second) {
			if (diagnostic != nullptr) {
				*diagnostic = "Connect field '" + field_name + "' is declared more than once.";
			}
			return {};
		}

		if (field_name == "id" || field_name == "type") {
			std::string *destination = field_name == "id" ? &connection_id : &connection_type;
			if (!SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, destination, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else if (field_name == "source" || field_name == "target") {
			std::string *object_destination =
				field_name == "source" ? &source_object_id : &target_object_id;
			std::string *interface_destination =
				field_name == "source" ? &source_interface_id : &target_interface_id;
			if (!SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, object_destination, diagnostic) ||
			    !SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, interface_destination, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else if (field_name == "clearance") {
			if (!SpatialDeclarationFieldReader::readClearance(
					raw_tokens,
					index,
					field_start_index,
					expression_parser,
					source_range_resolver,
					&clearance,
					diagnostic)) {
				return {};
			}
		}
		else if (field_name == "insertionDepth") {
			if (!SpatialDeclarationFieldReader::readExpression(
					raw_tokens,
					index,
					field_name,
					expression_parser,
					&insertion_depth,
					diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else {
			if (diagnostic != nullptr) {
				*diagnostic = "Connect does not support field '" + field_name + "'.";
			}
			return {};
		}
	}

	if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
		if (diagnostic != nullptr) *diagnostic = "Connect is missing its closing parenthesis.";
		return {};
	}
	++(*index);
	if (connection_id.empty() || source_object_id.empty() || source_interface_id.empty() ||
	    target_object_id.empty() || target_interface_id.empty() || connection_type.empty()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Connect requires id(...), source(...), target(...), and type(...).";
		}
		return {};
	}

	return std::make_shared<const SpatialConnectionDeclarationSyntax>(
		std::move(connection_id),
		std::move(source_object_id),
		std::move(source_interface_id),
		std::move(target_object_id),
		std::move(target_interface_id),
		std::move(connection_type),
		std::move(clearance),
		std::move(insertion_depth),
		SpatialDeclarationFieldReader::resolveRange(
			source_range_resolver, declaration_start_index, *index));
}
