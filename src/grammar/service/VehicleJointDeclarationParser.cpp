#include "grammar/service/VehicleJointDeclarationParser.h"

#include "SpatialDeclarationFieldReader.h"

#include <set>
#include <utility>

std::shared_ptr<const VehicleJointDeclarationSyntax>
VehicleJointDeclarationParser::parse(
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	std::size_t declaration_start_index,
	const SpatialDeclarationParserTypes::ExpressionArgumentParser &expression_parser,
	const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
	std::string *diagnostic) const
{
	if (!SpatialDeclarationFieldReader::beginDeclaration(
			raw_tokens, index, "Joint", diagnostic)) {
		return {};
	}

	std::string joint_identifier;
	std::string joint_type;
	std::string source_object_identifier;
	std::string source_interface_identifier;
	std::string target_object_identifier;
	std::string target_interface_identifier;
	std::array<GeometryExpression, 3> axis{
		GeometryExpression("0"), GeometryExpression("1"), GeometryExpression("0")};
	GeometryExpression minimum_state("0");
	GeometryExpression maximum_state("0");
	GeometryExpression current_state("0");
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
				*diagnostic = "Joint field '" + field_name +
				              "' is declared more than once.";
			}
			return {};
		}

		if (field_name == "id" || field_name == "type") {
			std::string *destination =
				field_name == "id" ? &joint_identifier : &joint_type;
			if (!SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, destination, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else if (field_name == "source" || field_name == "target") {
			std::string *object_destination = field_name == "source"
				? &source_object_identifier : &target_object_identifier;
			std::string *interface_destination = field_name == "source"
				? &source_interface_identifier : &target_interface_identifier;
			if (!SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, object_destination, diagnostic) ||
			    !SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, interface_destination, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else if (field_name == "axis") {
			if (!SpatialDeclarationFieldReader::readVector3(
					raw_tokens, index, field_name, expression_parser, &axis, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else if (field_name == "limits") {
			if (!SpatialDeclarationFieldReader::readExpression(
					raw_tokens, index, field_name, expression_parser,
					&minimum_state, diagnostic) ||
			    !SpatialDeclarationFieldReader::readExpression(
					raw_tokens, index, field_name, expression_parser,
					&maximum_state, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else if (field_name == "state") {
			if (!SpatialDeclarationFieldReader::readExpression(
					raw_tokens, index, field_name, expression_parser,
					&current_state, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else {
			if (diagnostic != nullptr) {
				*diagnostic = "Joint does not support field '" + field_name + "'.";
			}
			return {};
		}
	}

	if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
		if (diagnostic != nullptr) *diagnostic = "Joint is missing its closing parenthesis.";
		return {};
	}
	++(*index);
	if (joint_identifier.empty() || joint_type.empty() ||
	    source_object_identifier.empty() || source_interface_identifier.empty() ||
	    target_object_identifier.empty() || target_interface_identifier.empty()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Joint requires id(...), type(...), source(...), and target(...).";
		}
		return {};
	}

	return std::make_shared<const VehicleJointDeclarationSyntax>(
		std::move(joint_identifier), std::move(joint_type),
		std::move(source_object_identifier), std::move(source_interface_identifier),
		std::move(target_object_identifier), std::move(target_interface_identifier),
		std::move(axis), std::move(minimum_state), std::move(maximum_state),
		std::move(current_state), SpatialDeclarationFieldReader::resolveRange(
			source_range_resolver, declaration_start_index, *index));
}
