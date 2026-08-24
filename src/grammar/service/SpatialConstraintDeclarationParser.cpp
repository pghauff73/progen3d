#include "grammar/service/SpatialConstraintDeclarationParser.h"

#include "SpatialDeclarationFieldReader.h"

#include <array>
#include <set>
#include <utility>

std::shared_ptr<const SpatialConstraintDeclarationSyntax>
SpatialConstraintDeclarationParser::parse(
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	std::size_t declaration_start_index,
	const SpatialDeclarationParserTypes::ExpressionArgumentParser &expression_parser,
	const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
	std::string *diagnostic) const
{
	if (!SpatialDeclarationFieldReader::beginDeclaration(
			raw_tokens, index, "Position", diagnostic)) {
		return {};
	}

	std::string constraint_id;
	std::string moving_interface_id;
	std::string target_object_id;
	std::string target_interface_id;
	std::string mode;
	SpatialDirectionSyntax direction(
		"TargetInterface",
		{GeometryExpression("0"), GeometryExpression("0"), GeometryExpression("-1")},
		{});
	SpatialClearanceSyntax clearance;
	GeometryExpression seating_depth("0");
	GeometryExpression maximum_distance("1");
	GeometryExpression tolerance("0.0005");
	GeometryExpression priority("0");
	std::vector<std::string> collision_mask;
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
				*diagnostic = "Position field '" + field_name + "' is declared more than once.";
			}
			return {};
		}

		if (field_name == "id" || field_name == "moving" || field_name == "mode") {
			std::string *destination = field_name == "id" ? &constraint_id
			                         : field_name == "moving" ? &moving_interface_id
			                                                    : &mode;
			if (!SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, destination, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else if (field_name == "target") {
			if (!SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, &target_object_id, diagnostic) ||
			    !SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, &target_interface_id, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else if (field_name == "direction") {
			std::string frame_name;
			std::array<GeometryExpression, 3> vector_expressions{
				GeometryExpression("0"), GeometryExpression("0"), GeometryExpression("0")};
			if (!SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, &frame_name, diagnostic) ||
			    !SpatialDeclarationFieldReader::readVector3(
					raw_tokens,
					index,
					field_name,
					expression_parser,
					&vector_expressions,
					diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
			direction = SpatialDirectionSyntax(
				std::move(frame_name),
				std::move(vector_expressions),
				SpatialDeclarationFieldReader::resolveRange(
					source_range_resolver, field_start_index, *index));
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
		else if (field_name == "seatingDepth" || field_name == "maxDistance" ||
		         field_name == "tolerance" || field_name == "priority") {
			GeometryExpression *destination =
				field_name == "seatingDepth" ? &seating_depth
				: field_name == "maxDistance" ? &maximum_distance
				: field_name == "tolerance" ? &tolerance
				                              : &priority;
			if (!SpatialDeclarationFieldReader::readExpression(
					raw_tokens, index, field_name, expression_parser, destination, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else if (field_name == "mask") {
			if (!SpatialDeclarationFieldReader::readIdentifierList(
					raw_tokens, index, field_name, true, &collision_mask, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else {
			if (diagnostic != nullptr) {
				*diagnostic = "Position does not support field '" + field_name + "'.";
			}
			return {};
		}
	}

	if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
		if (diagnostic != nullptr) *diagnostic = "Position is missing its closing parenthesis.";
		return {};
	}
	++(*index);
	if (constraint_id.empty() || moving_interface_id.empty() || target_object_id.empty() ||
	    target_interface_id.empty() || mode.empty()) {
		if (diagnostic != nullptr) {
			*diagnostic = "Position requires id(...), moving(...), target(...), and mode(...).";
		}
		return {};
	}

	return std::make_shared<const SpatialConstraintDeclarationSyntax>(
		std::move(constraint_id),
		std::move(moving_interface_id),
		std::move(target_object_id),
		std::move(target_interface_id),
		std::move(mode),
		std::move(direction),
		std::move(clearance),
		std::move(seating_depth),
		std::move(maximum_distance),
		std::move(tolerance),
		std::move(priority),
		std::move(collision_mask),
		SpatialDeclarationFieldReader::resolveRange(
			source_range_resolver, declaration_start_index, *index));
}
