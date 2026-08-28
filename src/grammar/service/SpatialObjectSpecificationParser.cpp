#include "grammar/service/SpatialObjectSpecificationParser.h"

#include "SpatialDeclarationFieldReader.h"

#include <set>
#include <utility>

std::shared_ptr<const SpatialObjectDescriptorSyntax>
SpatialObjectSpecificationParser::parse(
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	std::size_t declaration_start_index,
	const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
	std::string *diagnostic) const
{
	if (!SpatialDeclarationFieldReader::beginDeclaration(
			raw_tokens, index, "Object", diagnostic)) {
		return {};
	}

	std::string object_id;
	std::string object_name;
	std::string object_class;
	std::string container_object_id;
	std::vector<std::string> taxonomy_segments;
	std::string collision_layer = "Temporary";
	std::vector<std::string> collision_mask;
	GrammarSourceRange taxonomy_range;
	GrammarSourceRange collision_range;
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
				*diagnostic = "Object field '" + field_name + "' is declared more than once.";
			}
			return {};
		}

		if (field_name == "id") {
			if (!SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, &object_id, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else if (field_name == "name") {
			if (!SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, &object_name, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else if (field_name == "class") {
			if (!SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, &object_class, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else if (field_name == "container") {
			if (!SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, &container_object_id, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
		}
		else if (field_name == "taxonomy") {
			if (!SpatialDeclarationFieldReader::readIdentifierList(
					raw_tokens, index, field_name, false, &taxonomy_segments, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
			taxonomy_range = SpatialDeclarationFieldReader::resolveRange(
				source_range_resolver, field_start_index, *index);
		}
		else if (field_name == "layer") {
			if (!SpatialDeclarationFieldReader::readIdentifier(
					raw_tokens, index, field_name, &collision_layer, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
			collision_range = SpatialDeclarationFieldReader::resolveRange(
				source_range_resolver, field_start_index, *index);
		}
		else if (field_name == "mask") {
			if (!SpatialDeclarationFieldReader::readIdentifierList(
					raw_tokens, index, field_name, true, &collision_mask, diagnostic) ||
			    !SpatialDeclarationFieldReader::endField(
					raw_tokens, index, field_name, diagnostic)) {
				return {};
			}
			collision_range = SpatialDeclarationFieldReader::resolveRange(
				source_range_resolver, field_start_index, *index);
		}
		else {
			if (diagnostic != nullptr) {
				*diagnostic = "Object does not support field '" + field_name + "'.";
			}
			return {};
		}
	}

	if (*index >= raw_tokens.size() || raw_tokens[*index] != ")") {
		if (diagnostic != nullptr) *diagnostic = "Object is missing its closing parenthesis.";
		return {};
	}
	++(*index);
	if (object_id.empty()) {
		if (diagnostic != nullptr) *diagnostic = "Object requires id(...).";
		return {};
	}
	if (object_class.empty()) {
		if (diagnostic != nullptr) *diagnostic = "Object requires class(...).";
		return {};
	}
	if (object_name.empty()) object_name = object_id;

	const GrammarSourceRange declaration_range =
		SpatialDeclarationFieldReader::resolveRange(
			source_range_resolver, declaration_start_index, *index);
	return std::make_shared<const SpatialObjectDescriptorSyntax>(
		std::move(object_id),
		std::move(object_name),
		std::move(object_class),
		SpatialTaxonomySyntax(std::move(taxonomy_segments), taxonomy_range),
		std::move(container_object_id),
		CollisionParticipationSyntax(
			std::move(collision_layer), std::move(collision_mask), collision_range),
		declaration_range);
}
