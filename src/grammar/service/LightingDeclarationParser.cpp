#include "grammar/service/LightingDeclarationParser.h"

#include "SpatialDeclarationFieldReader.h"

#include <set>

namespace {

bool read_declaration_id(const std::vector<std::string> &raw_tokens,
	                    std::size_t *index,
	                    const std::string &declaration_name,
	                    std::string *identifier,
	                    std::string *diagnostic)
{
	return SpatialDeclarationFieldReader::beginDeclaration(
			raw_tokens, index, declaration_name, diagnostic) &&
	       SpatialDeclarationFieldReader::readIdentifier(
			raw_tokens, index, declaration_name + " ID", identifier, diagnostic);
}

bool read_vector2(
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	const std::string &field_name,
	const SpatialDeclarationParserTypes::ExpressionArgumentParser &expression_parser,
	std::array<GeometryExpression, 2> *value,
	std::string *diagnostic)
{
	return SpatialDeclarationFieldReader::readExpression(
			raw_tokens, index, field_name, expression_parser, &(*value)[0], diagnostic) &&
	       SpatialDeclarationFieldReader::readExpression(
			raw_tokens, index, field_name, expression_parser, &(*value)[1], diagnostic);
}

bool close_declaration(const std::vector<std::string> &raw_tokens,
	                  std::size_t *index,
	                  const std::string &declaration_name,
	                  std::string *diagnostic)
{
	if (index == nullptr || *index >= raw_tokens.size() || raw_tokens[*index] != ")") {
		if (diagnostic != nullptr) {
			*diagnostic = declaration_name + " is missing its closing parenthesis.";
		}
		return false;
	}
	++(*index);
	return true;
}

}

std::shared_ptr<const SceneLightDeclarationSyntax> LightingDeclarationParser::parseLight(
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	std::size_t declaration_start_index,
	const SpatialDeclarationParserTypes::ExpressionArgumentParser &expression_parser,
	const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
	std::string *diagnostic) const
{
	auto syntax = std::make_shared<SceneLightDeclarationSyntax>();
	if (!read_declaration_id(raw_tokens, index, "Light", &syntax->light_id, diagnostic)) return {};
	std::set<std::string> fields;
	while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
		std::string field;
		std::size_t field_start = 0;
		if (!SpatialDeclarationFieldReader::beginField(raw_tokens, index, &field, &field_start, diagnostic) ||
		    !fields.insert(field).second) return {};
		bool read = false;
		if (field == "type" || field == "shadow" || field == "reference" || field == "twoSided") {
			std::string *destination = field == "type" ? &syntax->type
				: field == "shadow" ? &syntax->shadow
				: field == "reference" ? &syntax->reference_frame
				: &syntax->two_sided;
			read = SpatialDeclarationFieldReader::readIdentifier(raw_tokens, index, field, destination, diagnostic);
		}
		else if (field == "position" || field == "direction" || field == "color") {
			auto *destination = field == "position" ? &syntax->position
				: field == "direction" ? &syntax->direction : &syntax->color;
			read = SpatialDeclarationFieldReader::readVector3(raw_tokens, index, field, expression_parser, destination, diagnostic);
		}
		else if (field == "cone" || field == "area") {
			read = read_vector2(raw_tokens, index, field, expression_parser,
				field == "cone" ? &syntax->cone : &syntax->area, diagnostic);
		}
		else if (field == "temperature" || field == "range" || field == "exposure" ||
		         field == "lumens" || field == "candela" || field == "lux") {
			GeometryExpression *destination = field == "temperature" ? &syntax->temperature
				: field == "range" ? &syntax->range
				: field == "exposure" ? &syntax->exposure : &syntax->intensity;
			read = SpatialDeclarationFieldReader::readExpression(raw_tokens, index, field, expression_parser, destination, diagnostic);
			if (field == "temperature") syntax->uses_temperature = true;
			if (field == "lumens") syntax->intensity_unit = "Lumens";
			if (field == "candela") syntax->intensity_unit = "Candela";
			if (field == "lux") syntax->intensity_unit = "Lux";
		}
		else {
			if (diagnostic != nullptr) *diagnostic = "Light does not support field '" + field + "'.";
			return {};
		}
		if (!read || !SpatialDeclarationFieldReader::endField(raw_tokens, index, field, diagnostic)) return {};
	}
	if (!close_declaration(raw_tokens, index, "Light", diagnostic)) return {};
	syntax->source_range = SpatialDeclarationFieldReader::resolveRange(
		source_range_resolver, declaration_start_index, *index);
	return syntax;
}

std::shared_ptr<const LightFixtureDeclarationSyntax> LightingDeclarationParser::parseFixture(
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	std::size_t declaration_start_index,
	const SpatialDeclarationParserTypes::ExpressionArgumentParser &expression_parser,
	const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
	std::string *diagnostic) const
{
	auto syntax = std::make_shared<LightFixtureDeclarationSyntax>();
	if (!read_declaration_id(raw_tokens, index, "LightFixture", &syntax->fixture_id, diagnostic)) return {};
	std::set<std::string> fields;
	while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
		std::string field;
		std::size_t field_start = 0;
		if (!SpatialDeclarationFieldReader::beginField(raw_tokens, index, &field, &field_start, diagnostic) ||
		    !fields.insert(field).second) return {};
		bool read = false;
		if (field == "type" || field == "emitter" || field == "mount" || field == "shadow") {
			std::string *destination = field == "type" ? &syntax->fixture_type
				: field == "emitter" ? &syntax->emitter_id
				: field == "mount" ? &syntax->mount_interface : &syntax->shadow;
			read = SpatialDeclarationFieldReader::readIdentifier(raw_tokens, index, field, destination, diagnostic);
		}
		else if (field == "position" || field == "direction" || field == "clearance") {
			auto *destination = field == "position" ? &syntax->position
				: field == "direction" ? &syntax->direction : &syntax->clearance;
			read = SpatialDeclarationFieldReader::readVector3(raw_tokens, index, field, expression_parser, destination, diagnostic);
		}
		else if (field == "cone") {
			read = read_vector2(raw_tokens, index, field, expression_parser, &syntax->cone, diagnostic);
		}
		else if (field == "temperature" || field == "lumens" || field == "range") {
			GeometryExpression *destination = field == "temperature" ? &syntax->temperature
				: field == "lumens" ? &syntax->lumens : &syntax->range;
			read = SpatialDeclarationFieldReader::readExpression(raw_tokens, index, field, expression_parser, destination, diagnostic);
		}
		else {
			if (diagnostic != nullptr) *diagnostic = "LightFixture does not support field '" + field + "'.";
			return {};
		}
		if (!read || !SpatialDeclarationFieldReader::endField(raw_tokens, index, field, diagnostic)) return {};
	}
	if (!close_declaration(raw_tokens, index, "LightFixture", diagnostic)) return {};
	if (syntax->emitter_id.empty()) syntax->emitter_id = syntax->fixture_id + "Emitter";
	syntax->source_range = SpatialDeclarationFieldReader::resolveRange(source_range_resolver, declaration_start_index, *index);
	return syntax;
}

std::shared_ptr<const LightSwitchDeclarationSyntax> LightingDeclarationParser::parseSwitch(
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	std::size_t declaration_start_index,
	const SpatialDeclarationParserTypes::ExpressionArgumentParser &expression_parser,
	const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
	std::string *diagnostic) const
{
	auto syntax = std::make_shared<LightSwitchDeclarationSyntax>();
	if (!read_declaration_id(raw_tokens, index, "LightSwitch", &syntax->switch_id, diagnostic)) return {};
	std::set<std::string> fields;
	while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
		std::string field;
		std::size_t field_start = 0;
		if (!SpatialDeclarationFieldReader::beginField(raw_tokens, index, &field, &field_start, diagnostic) ||
		    !fields.insert(field).second) return {};
		bool read = false;
		if (field == "type" || field == "mount" || field == "state") {
			std::string *destination = field == "type" ? &syntax->switch_type
				: field == "mount" ? &syntax->mount_object_id : &syntax->state;
			read = SpatialDeclarationFieldReader::readIdentifier(raw_tokens, index, field, destination, diagnostic);
		}
		else if (field == "position") {
			read = SpatialDeclarationFieldReader::readVector3(raw_tokens, index, field, expression_parser, &syntax->position, diagnostic);
		}
		else if (field == "height" || field == "dimmer") {
			read = SpatialDeclarationFieldReader::readExpression(raw_tokens, index, field, expression_parser,
				field == "height" ? &syntax->height : &syntax->dimmer, diagnostic);
		}
		else {
			if (diagnostic != nullptr) *diagnostic = "LightSwitch does not support field '" + field + "'.";
			return {};
		}
		if (!read || !SpatialDeclarationFieldReader::endField(raw_tokens, index, field, diagnostic)) return {};
	}
	if (!close_declaration(raw_tokens, index, "LightSwitch", diagnostic)) return {};
	syntax->source_range = SpatialDeclarationFieldReader::resolveRange(source_range_resolver, declaration_start_index, *index);
	return syntax;
}

std::shared_ptr<const LightingArrayDeclarationSyntax> LightingDeclarationParser::parseArray(
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	std::size_t declaration_start_index,
	const SpatialDeclarationParserTypes::ExpressionArgumentParser &expression_parser,
	const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
	std::string *diagnostic) const
{
	auto syntax = std::make_shared<LightingArrayDeclarationSyntax>();
	if (!read_declaration_id(raw_tokens, index, "LightingArray", &syntax->array_id, diagnostic)) return {};
	std::set<std::string> fields;
	while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
		std::string field;
		std::size_t field_start = 0;
		if (!SpatialDeclarationFieldReader::beginField(raw_tokens, index, &field, &field_start, diagnostic) ||
		    !fields.insert(field).second) return {};
		bool read = false;
		if (field == "fixture" || field == "mount" || field == "shadow" || field == "circuit") {
			std::string *destination = field == "fixture" ? &syntax->fixture_type
				: field == "mount" ? &syntax->mount_interface
				: field == "shadow" ? &syntax->shadow : &syntax->circuit_id;
			read = SpatialDeclarationFieldReader::readIdentifier(
				raw_tokens, index, field, destination, diagnostic);
		}
		else if (field == "origin" || field == "direction" || field == "clearance") {
			auto *destination = field == "origin" ? &syntax->origin
				: field == "direction" ? &syntax->direction : &syntax->clearance;
			read = SpatialDeclarationFieldReader::readVector3(
				raw_tokens, index, field, expression_parser, destination, diagnostic);
		}
		else if (field == "grid" || field == "spacing" || field == "cone") {
			auto *destination = field == "grid" ? &syntax->grid
				: field == "spacing" ? &syntax->spacing : &syntax->cone;
			read = read_vector2(
				raw_tokens, index, field, expression_parser, destination, diagnostic);
		}
		else if (field == "temperature" || field == "lumens" || field == "range") {
			GeometryExpression *destination = field == "temperature" ? &syntax->temperature
				: field == "lumens" ? &syntax->lumens : &syntax->range;
			read = SpatialDeclarationFieldReader::readExpression(
				raw_tokens, index, field, expression_parser, destination, diagnostic);
		}
		else if (field == "controls") {
			read = SpatialDeclarationFieldReader::readIdentifierList(
				raw_tokens, index, field, false, &syntax->control_ids, diagnostic);
		}
		else {
			if (diagnostic != nullptr) {
				*diagnostic = "LightingArray does not support field '" + field + "'.";
			}
			return {};
		}
		if (!read || !SpatialDeclarationFieldReader::endField(raw_tokens, index, field, diagnostic)) return {};
	}
	if (!close_declaration(raw_tokens, index, "LightingArray", diagnostic)) return {};
	syntax->source_range = SpatialDeclarationFieldReader::resolveRange(
		source_range_resolver, declaration_start_index, *index);
	return syntax;
}

std::shared_ptr<const LightingCircuitDeclarationSyntax> LightingDeclarationParser::parseCircuit(
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	std::size_t declaration_start_index,
	const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
	std::string *diagnostic) const
{
	auto syntax = std::make_shared<LightingCircuitDeclarationSyntax>();
	if (!read_declaration_id(raw_tokens, index, "LightingCircuit", &syntax->circuit_id, diagnostic)) return {};
	std::set<std::string> fields;
	while (*index < raw_tokens.size() && raw_tokens[*index] != ")") {
		std::string field;
		std::size_t field_start = 0;
		if (!SpatialDeclarationFieldReader::beginField(raw_tokens, index, &field, &field_start, diagnostic) ||
		    !fields.insert(field).second) return {};
		bool read = false;
		if (field == "controls" || field == "fixtures") {
			read = SpatialDeclarationFieldReader::readIdentifierList(raw_tokens, index, field, false,
				field == "controls" ? &syntax->control_ids : &syntax->fixture_ids, diagnostic);
		}
		else if (field == "alwaysOn") {
			read = SpatialDeclarationFieldReader::readIdentifier(raw_tokens, index, field, &syntax->always_on, diagnostic);
		}
		else {
			if (diagnostic != nullptr) *diagnostic = "LightingCircuit does not support field '" + field + "'.";
			return {};
		}
		if (!read || !SpatialDeclarationFieldReader::endField(raw_tokens, index, field, diagnostic)) return {};
	}
	if (!close_declaration(raw_tokens, index, "LightingCircuit", diagnostic)) return {};
	syntax->source_range = SpatialDeclarationFieldReader::resolveRange(source_range_resolver, declaration_start_index, *index);
	return syntax;
}

std::shared_ptr<const ControlsDeclarationSyntax> LightingDeclarationParser::parseControls(
	const std::vector<std::string> &raw_tokens,
	std::size_t *index,
	std::size_t declaration_start_index,
	const SpatialDeclarationParserTypes::SourceRangeResolver &source_range_resolver,
	std::string *diagnostic) const
{
	auto syntax = std::make_shared<ControlsDeclarationSyntax>();
	if (!read_declaration_id(raw_tokens, index, "Controls", &syntax->control_id, diagnostic)) return {};
	if (!SpatialDeclarationFieldReader::readIdentifierList(
			raw_tokens, index, "Controls targets", false, &syntax->light_ids, diagnostic) ||
	    !close_declaration(raw_tokens, index, "Controls", diagnostic)) {
		return {};
	}
	syntax->source_range = SpatialDeclarationFieldReader::resolveRange(source_range_resolver, declaration_start_index, *index);
	return syntax;
}
