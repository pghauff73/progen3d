#include "vehicle/mcsmv2/service/ModernCarSemanticGrammarLoweringService.h"

#include "vehicle/mcsmv2/service/VehicleWheelGrammarLoweringService.h"

#include <array>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace {

std::string number(double value)
{
	if (std::fabs(value) < 5.0e-10) value = 0.0;
	std::ostringstream stream;
	stream << std::fixed << std::setprecision(6) << value;
	std::string text = stream.str();
	while (!text.empty() && text.back() == '0') text.pop_back();
	if (!text.empty() && text.back() == '.') text.pop_back();
	return text.empty() || text == "-0" ? "0" : text;
}

std::string grammarSymbol(const std::string &identifier)
{
	std::string symbol;
	bool capitalize = true;
	for (unsigned char character : identifier) {
		if (!std::isalnum(character)) {
			capitalize = true;
			continue;
		}
		symbol.push_back(capitalize
			? static_cast<char>(std::toupper(character))
			: static_cast<char>(character));
		capitalize = false;
	}
	return symbol.empty() ? "Variant" : symbol;
}

std::string objectIdentifier(const std::string &identifier)
{
	std::string result;
	for (unsigned char character : identifier) {
		result.push_back(std::isalnum(character)
			? static_cast<char>(std::toupper(character))
			: '_');
	}
	return result;
}

std::string bodyMaterial(const ModernCarSemanticVariant &variant)
{
	const VehicleBodyColor &color = variant.bodyColor();
	if (color.blue() > color.red() && color.blue() > color.green()) {
		return "bluepaint";
	}
	if (color.red() > 170 && color.green() > 170 && color.blue() > 170) {
		return "silverbluepolishedmetal";
	}
	return "redmetallicpaint";
}

std::string meshKey(const ModernCarSemanticVariant &variant)
{
	if (variant.identifier() == "reference") return "MCSMv2ReferenceBodyMesh";
	if (variant.identifier() == "track") return "MCSMv2TrackBodyMesh";
	if (variant.identifier() == "aero") return "MCSMv2AeroBodyMesh";
	if (variant.identifier() == "crossover") return "MCSMv2CrossoverBodyMesh";
	throw std::invalid_argument(
		"No generated mesh key is registered for MCSMv2 variant '" +
		variant.identifier() + "'.");
}

std::string lowerBodyRule(
	const ModernCarSemanticVariant &variant,
	const std::string &rule_name)
{
	std::ostringstream stream;
	stream << rule_name << " ->\n"
	       << "[\n"
	       << "    !I(GeneratedMeshReference(\n"
	       << "          meshKey(" << meshKey(variant) << ")\n"
	       << "          detail(LOD2))\n"
	       << "       material(" << bodyMaterial(variant)
	       << ") alpha(1) texscale(0.16))\n"
	       << "]\n";
	return stream.str();
}

} // namespace

std::string ModernCarSemanticGrammarLoweringService::lowerFamilyPreview(
	const ModernCarSemanticFamily &family) const
{
	std::ostringstream stream;
	stream << "# MCSMv2 executable modern-car family preview.\n"
	       << "# +X right, +Y up, +Z forward; each vehicle is grounded at Y=0.\n\n"
	       << "Start ->\n"
	       << "    MCSMv2ExecutableFamilyPreview\n\n"
	       << "MCSMv2ExecutableFamilyPreview ->\n"
	       << "[\n"
	       << "    Object(id(MCSMV2_FAMILY_PREVIEW) name(McsMv2FamilyPreview)\n"
	       << "           class(ModernCarSemanticFamily)\n"
	       << "           taxonomy(Vehicle MCSMv2 ParametricCandidate Family)\n"
	       << "           layer(Equipment) mask(Equipment Terrain Temporary))\n"
	       << "    [\n";
	const double spacing = 3.1;
	const double first_position =
		-static_cast<double>(family.variants().size() - 1u) * spacing * 0.5;
	for (std::size_t index = 0u; index < family.variants().size(); ++index) {
		const ModernCarSemanticVariant &variant = family.variants()[index];
		stream << "        [ T(" << number(first_position + spacing * index)
		       << " 0 0) MCSMv2" << grammarSymbol(variant.identifier())
		       << "Preview ]\n";
	}
	stream << "    ]\n"
	       << "]\n\n";
	for (const ModernCarSemanticVariant &variant : family.variants()) {
		const std::string symbol = "MCSMv2" + grammarSymbol(variant.identifier());
		stream << lowerVariantRule(
			variant,
			symbol + "Preview",
			objectIdentifier("MCSMv2_" + variant.identifier()),
			"MCSMV2_FAMILY_PREVIEW")
		       << '\n'
		       << lowerBodyRule(variant, symbol + "Body") << '\n'
		       << VehicleWheelGrammarLoweringService().lowerWheelRule(
				variant,
				symbol + "Wheel") << '\n';
	}
	return stream.str();
}

std::string ModernCarSemanticGrammarLoweringService::lowerVariantPreview(
	const ModernCarSemanticVariant &variant) const
{
	const std::string symbol = "MCSMv2" + grammarSymbol(variant.identifier());
	std::ostringstream stream;
	stream << "# MCSMv2 executable " << variant.identifier() << " preview.\n"
	       << "# +X right, +Y up, +Z forward; origin is ground package centre.\n\n"
	       << "Start ->\n"
	       << "    " << symbol << "Preview\n\n"
	       << lowerVariantRule(
			variant,
			symbol + "Preview",
			objectIdentifier("MCSMv2_" + variant.identifier()),
			std::string())
	       << '\n'
	       << lowerBodyRule(variant, symbol + "Body") << '\n'
	       << VehicleWheelGrammarLoweringService().lowerWheelRule(
			variant,
			symbol + "Wheel");
	return stream.str();
}

std::string ModernCarSemanticGrammarLoweringService::lowerVariantRule(
	const ModernCarSemanticVariant &variant,
	const std::string &rule_name,
	const std::string &object_identifier,
	const std::string &container_identifier) const
{
	const VehiclePackageParameters &package = variant.package();
	const VehicleWheelParameters &wheels = variant.wheelMotion().wheelGeometry();
	const std::string wheel_rule =
		"MCSMv2" + grammarSymbol(variant.identifier()) + "Wheel";
	const std::string body_rule =
		"MCSMv2" + grammarSymbol(variant.identifier()) + "Body";
	std::ostringstream stream;
	stream << rule_name << " ->\n"
	       << "[\n"
	       << "    Object(id(" << object_identifier << ")\n"
	       << "           name(" << grammarSymbol(variant.displayName()) << ")\n"
	       << "           class(ModernCarSemanticVariant)\n";
	if (!container_identifier.empty()) {
		stream << "           container(" << container_identifier << ")\n";
	}
	stream
	       << "           taxonomy(Vehicle MCSMv2 ParametricCandidate)\n"
	       << "           layer(Equipment) mask(Equipment Terrain Temporary))\n"
	       << "    [\n"
	       << "        Interface(id(packageCentre) type(InspectionInterface)\n"
	       << "                  origin(0 0 0) normal(0 1 0) tangent(0 0 1)\n"
	       << "                  region(Point) tolerance(0.001))\n"
	       << "        " << body_rule << "\n"
	       << "        " << wheel_rule << "(" << number(-wheels.frontTrack() * 0.5)
	       << ' ' << number(package.frontAxleStation()) << " -1)\n"
	       << "        " << wheel_rule << "(" << number(wheels.frontTrack() * 0.5)
	       << ' ' << number(package.frontAxleStation()) << " 1)\n"
	       << "        " << wheel_rule << "(" << number(-wheels.rearTrack() * 0.5)
	       << ' ' << number(package.rearAxleStation()) << " -1)\n"
	       << "        " << wheel_rule << "(" << number(wheels.rearTrack() * 0.5)
	       << ' ' << number(package.rearAxleStation()) << " 1)\n"
	       << "    ]\n"
	       << "]\n";
	return stream.str();
}
