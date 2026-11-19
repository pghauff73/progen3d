#include "grammar/service/CoaxialDriveParser.h"
#include "SpatialDeclarationFieldReader.h"

#include <set>
#include <sstream>

std::string CoaxialDriveSyntax::canonicalText() const
{
	std::ostringstream out;
	out << "CoaxialDrive(";
	auto write = [&](const char *name, const auto &values) {
		out << name << '(';
		for (std::size_t i = 0; i < values.size(); ++i) {
			if (i) out << ' ';
			out << values[i].sourceText();
		}
		out << ')';
	};
	write("hoop", hoop); out << ' ';
	write("motion", motion); out << ' ';
	write("transmission", transmission); out << ' ';
	write("motor", motor); out << ')';
	return out.str();
}

std::shared_ptr<const CoaxialDriveSyntax> CoaxialDriveParser::parse(
	const std::vector<std::string> &tokens, std::size_t *index,
	const SpatialDeclarationParserTypes::ExpressionArgumentParser &expressions,
	std::string *diagnostic) const
{
	using Reader = SpatialDeclarationFieldReader;
	if (!Reader::beginDeclaration(tokens, index, "CoaxialDrive", diagnostic)) return {};
	auto drive = std::make_shared<CoaxialDriveSyntax>();
	std::set<std::string> seen;
	while (*index < tokens.size() && tokens[*index] != ")") {
		std::string field;
		std::size_t start;
		if (!Reader::beginField(tokens, index, &field, &start, diagnostic)) return {};
		if (!seen.insert(field).second) {
			*diagnostic = "Duplicate CoaxialDrive field '" + field + "'.";
			return {};
		}
		auto read = [&](auto &values) {
			for (auto &expression : values)
				if (!Reader::readExpression(tokens, index, field, expressions, &expression, diagnostic)) return false;
			return Reader::endField(tokens, index, field, diagnostic);
		};
		if (field == "hoop") { if (!read(drive->hoop)) return {}; }
		else if (field == "motion") { if (!read(drive->motion)) return {}; }
		else if (field == "transmission") { if (!read(drive->transmission)) return {}; }
		else if (field == "motor") { if (!read(drive->motor)) return {}; }
		else {
			*diagnostic = "Unknown CoaxialDrive field '" + field + "'.";
			return {};
		}
	}
	if (!Reader::endField(tokens, index, "CoaxialDrive", diagnostic)) return {};
	for (const char *field : {"hoop", "motion", "transmission", "motor"}) {
		if (!seen.count(field)) {
			*diagnostic = std::string("CoaxialDrive requires ") + field + "(...).";
			return {};
		}
	}
	return drive;
}
