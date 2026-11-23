#include "grammar/service/MemberStudyParser.h"
#include "SpatialDeclarationFieldReader.h"
#include <set>

std::shared_ptr<const MemberStudySyntax> MemberStudyParser::parse(
	const std::vector<std::string> &tokens, std::size_t *index,
	const SpatialDeclarationParserTypes::ExpressionArgumentParser &expressions,
	std::string *diagnostic) const
{
	using Reader = SpatialDeclarationFieldReader;
	if (!Reader::beginDeclaration(tokens, index, "PinRollerMember", diagnostic)) return {};
	auto member = std::make_shared<MemberStudySyntax>();
	std::set<std::string> seen;
	while (*index < tokens.size() && tokens[*index] != ")") {
		std::string field;
		std::size_t start;
		if (!Reader::beginField(tokens, index, &field, &start, diagnostic)) return {};
		if (!seen.insert(field).second) {
			*diagnostic = "Duplicate PinRollerMember field '" + field + "'.";
			return {};
		}
		if (field == "supports") {
			if (!Reader::readIdentifier(tokens, index, field, &member->pin, diagnostic) ||
			    !Reader::readIdentifier(tokens, index, field, &member->roller, diagnostic)) return {};
		} else if (field == "material") {
			if (!Reader::readIdentifier(tokens, index, field, &member->material, diagnostic) ||
			    !Reader::readExpression(tokens, index, field, expressions, &member->modulus, diagnostic) ||
			    !Reader::readExpression(tokens, index, field, expressions, &member->density, diagnostic)) return {};
		} else if (field == "metresPerUnit") {
			if (!Reader::readExpression(tokens, index, field, expressions, &member->metresPerUnit, diagnostic)) return {};
		} else if (field == "section" || field == "load") {
			auto &values = field == "section" ? member->section : member->load;
			if (!Reader::readVector3(tokens, index, field, expressions, &values, diagnostic)) return {};
		} else {
			*diagnostic = "Unknown PinRollerMember field '" + field + "'.";
			return {};
		}
		if (!Reader::endField(tokens, index, field, diagnostic)) return {};
	}
	if (!Reader::endField(tokens, index, "PinRollerMember", diagnostic)) return {};
	for (const char *required : {"supports", "metresPerUnit", "material", "section", "load"}) {
		if (!seen.count(required)) {
			*diagnostic = std::string("PinRollerMember requires ") + required + "(...).";
			return {};
		}
	}
	return member;
}
