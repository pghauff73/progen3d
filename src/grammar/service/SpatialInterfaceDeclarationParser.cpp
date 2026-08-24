#include "grammar/service/SpatialInterfaceDeclarationParser.h"

#include "SpatialDeclarationFieldReader.h"

#include <array>
#include <set>
#include <utility>

std::shared_ptr<const SpatialInterfaceDeclarationSyntax>
SpatialInterfaceDeclarationParser::parse(
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	std::size_t declaration_start_index,
	const SpatialDeclarationParserTypes::ExpressionArgumentParser &expression_parser,
	const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
	std::string *diagnostic) const
{
	if (!SpatialDeclarationFieldReader::beginDeclaration(
			raw_tokens, index, "Interface", diagnostic)) {
		return {};
	}

	std::string interface_id;
	std::string interface_type;
	std::array<GeometryExpression, 3> origin{
		GeometryExpression("0"), GeometryExpression("0"), GeometryExpression("0")};
	std::array<GeometryExpression, 3> normal{
		GeometryExpression("0"), GeometryExpression("1"), GeometryExpression("0")};
	std::array<GeometryExpression, 3> tangent{
		GeometryExpression("1"), GeometryExpression("0"), GeometryExpression("0")};
	std::string region_kind = "Point";
	std::vector<GeometryExpression> region_extents;
	std::string boundary_face_name;
	GeometryExpression tolerance("0.0005");
	SpatialClearanceSyntax clearance;
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
				*diagnostic = "Interface field '" + field_name + "' is declared more than once.";
			}
			return {};
		}

		if (field_name == "id" || field_name == "type") {
			std::string *destination = field_name == "id" ? &interface_id : &interface_type;
			if (!SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, destination, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else if (field_name == "origin" || field_name == "normal" ||
		         field_name == "tangent") {
			auto *destination = field_name == "origin" ? &origin
			                  : field_name == "normal" ? &normal
			                                             : &tangent;
			if (!SpatialDeclarationFieldReader::readVector3(
					raw_tokens, index, field_name, expression_parser, destination, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else if (field_name == "region") {
			if (!SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, &region_kind, diagnostic)) {
				return {};
			}
			if (region_kind == "PlaneRectangle") {
				for (int extent_index = 0; extent_index < 2; ++extent_index) {
					GeometryExpression extent("0");
					if (!SpatialDeclarationFieldReader::readExpression(
							raw_tokens, index, field_name, expression_parser, &extent, diagnostic)) {
						return {};
					}
					region_extents.push_back(std::move(extent));
				}
			}
			else if (region_kind == "AxisSegment") {
				GeometryExpression extent("0");
				if (!SpatialDeclarationFieldReader::readExpression(
						raw_tokens, index, field_name, expression_parser, &extent, diagnostic)) {
					return {};
				}
				region_extents.push_back(std::move(extent));
			}
			else if (region_kind == "ObjectBoundaryFace") {
				if (!SpatialDeclarationFieldReader::readIdentifier(
						raw_tokens, index, field_name, &boundary_face_name, diagnostic)) {
					return {};
				}
			}
			else if (region_kind != "Point") {
				if (diagnostic != nullptr) {
					*diagnostic = "Interface region kind '" + region_kind + "' is not supported.";
				}
				return {};
			}
			if (!SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else if (field_name == "tolerance") {
			if (!SpatialDeclarationFieldReader::readExpression(
					raw_tokens, index, field_name, expression_parser, &tolerance, diagnostic) ||
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
		else {
			if (diagnostic != nullptr) {
				*diagnostic = "Interface does not support field '" + field_name + "'.";
			}
			return {};
		}
	}

	if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
		if (diagnostic != nullptr) *diagnostic = "Interface is missing its closing parenthesis.";
		return {};
	}
	++(*index);
	if (interface_id.empty() || interface_type.empty()) {
		if (diagnostic != nullptr) *diagnostic = "Interface requires id(...) and type(...).";
		return {};
	}

	return std::make_shared<const SpatialInterfaceDeclarationSyntax>(
		std::move(interface_id),
		std::move(interface_type),
		std::move(origin),
		std::move(normal),
		std::move(tangent),
		std::move(region_kind),
		std::move(region_extents),
		std::move(boundary_face_name),
		std::move(tolerance),
		std::move(clearance),
		SpatialDeclarationFieldReader::resolveRange(
			source_range_resolver, declaration_start_index, *index));
}
