#include "vehicle/mcsmv2/service/McsMv21SemanticGrammarLoweringService.h"

#include "vehicle/mcsmv2/service/VehicleWheelGrammarLoweringService.h"

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

std::string meshKeyPrefix(const std::string &variant_identifier)
{
	if (variant_identifier == "reference") return "MCSMv21Reference";
	if (variant_identifier == "track") return "MCSMv21Track";
	if (variant_identifier == "aero") return "MCSMv21Aero";
	if (variant_identifier == "crossover") return "MCSMv21Crossover";
	throw std::invalid_argument(
		"No MCSMv2.1 generated mesh key is registered for variant '" +
		variant_identifier + "'.");
}

const McsMv21SemanticVariantDefinition &findSemanticVariant(
	const McsMv21SemanticFamilyDefinition &family,
	const std::string &identifier)
{
	for (const McsMv21SemanticVariantDefinition &variant : family.variants()) {
		if (variant.identifier() == identifier) return variant;
	}
	throw std::invalid_argument(
		"MCSMv2.1 semantic catalog does not contain variant '" + identifier + "'.");
}

std::string generatedMeshRule(
	const std::string &rule_name,
	const std::string &mesh_key,
	const std::string &material,
	double alpha,
	double texture_scale)
{
	std::ostringstream stream;
	stream << rule_name << " ->\n"
	       << "[\n"
	       << "    !I(GeneratedMeshReference(\n"
	       << "          meshKey(" << mesh_key << ")\n"
	       << "          detail(LOD2)\n"
	       << "          topology(surface))\n"
	       << "       material(" << material << ") alpha(" << number(alpha)
	       << ") texscale(" << number(texture_scale) << "))\n"
	       << "]\n";
	return stream.str();
}

} // namespace

std::string McsMv21SemanticGrammarLoweringService::lowerFamilyPreview(
	const ModernCarSemanticFamily &base_family,
	const McsMv21SemanticFamilyDefinition &semantic_family) const
{
	std::ostringstream stream;
	stream << "# MCSMv2.1 executable registered-surface family preview.\n"
	       << "# +X right, +Y up, +Z forward; each vehicle is grounded at Y=0.\n\n"
	       << "Start ->\n"
	       << "    MCSMv21ExecutableFamilyPreview\n\n"
	       << "MCSMv21ExecutableFamilyPreview ->\n"
	       << "[\n"
	       << "    Object(id(MCSMV21_FAMILY_PREVIEW) name(McsMv21FamilyPreview)\n"
	       << "           class(ModernCarSemanticFamilyV21)\n"
	       << "           taxonomy(Vehicle MCSMv2_1 RegisteredSurface V2 Family)\n"
	       << "           layer(Equipment) mask(Equipment Terrain Temporary))\n"
	       << "    [\n";
	const double spacing = 3.1;
	const double first_position =
		-static_cast<double>(base_family.variants().size() - 1u) * spacing * 0.5;
	for (std::size_t index = 0u; index < base_family.variants().size(); ++index) {
		const ModernCarSemanticVariant &base_variant = base_family.variants()[index];
		stream << "        [ T(" << number(first_position + spacing * index)
		       << " 0 0) MCSMv21" << grammarSymbol(base_variant.identifier())
		       << "Preview ]\n";
	}
	stream << "    ]\n"
	       << "]\n\n";
	for (const ModernCarSemanticVariant &base_variant : base_family.variants()) {
		const McsMv21SemanticVariantDefinition &semantic_variant =
			findSemanticVariant(semantic_family, base_variant.identifier());
		const std::string symbol =
			"MCSMv21" + grammarSymbol(base_variant.identifier());
		const std::string mesh_key_prefix = meshKeyPrefix(base_variant.identifier());
		stream << lowerVariantRule(
			base_variant,
			semantic_variant,
			symbol + "Preview",
			objectIdentifier("MCSMv21_" + base_variant.identifier()),
			"MCSMV21_FAMILY_PREVIEW")
		       << '\n'
		       << generatedMeshRule(
				symbol + "FixedBody",
				mesh_key_prefix + "FixedBodyMesh",
				bodyMaterial(base_variant),
				1.0,
				0.16)
		       << '\n'
		       << generatedMeshRule(
				symbol + "PanelRegions",
				mesh_key_prefix + "PanelRegionMesh",
				bodyMaterial(base_variant),
				1.0,
				0.16)
		       << '\n'
		       << generatedMeshRule(
				symbol + "Glass",
				mesh_key_prefix + "GlassMesh",
				"smokytransparentglass",
				0.56,
				0.10)
		       << '\n'
		       << VehicleWheelGrammarLoweringService().lowerWheelRule(
				base_variant,
				symbol + "Wheel")
		       << '\n';
	}
	return stream.str();
}

std::string McsMv21SemanticGrammarLoweringService::lowerVariantPreview(
	const ModernCarSemanticVariant &base_variant,
	const McsMv21SemanticVariantDefinition &semantic_variant) const
{
	if (base_variant.identifier() != semantic_variant.identifier()) {
		throw std::invalid_argument(
			"MCSMv2.1 grammar lowering requires matching base and semantic variants.");
	}
	const std::string symbol =
		"MCSMv21" + grammarSymbol(base_variant.identifier());
	const std::string mesh_key_prefix = meshKeyPrefix(base_variant.identifier());
	std::ostringstream stream;
	stream << "# MCSMv2.1 executable " << base_variant.identifier()
	       << " registered-surface preview.\n"
	       << "# +X right, +Y up, +Z forward; origin is ground package centre.\n\n"
	       << "Start ->\n"
	       << "    " << symbol << "Preview\n\n"
	       << lowerVariantRule(
			base_variant,
			semantic_variant,
			symbol + "Preview",
			objectIdentifier("MCSMv21_" + base_variant.identifier()),
			std::string())
	       << '\n'
	       << generatedMeshRule(
			symbol + "FixedBody",
			mesh_key_prefix + "FixedBodyMesh",
			bodyMaterial(base_variant),
			1.0,
			0.16)
	       << '\n'
	       << generatedMeshRule(
			symbol + "PanelRegions",
			mesh_key_prefix + "PanelRegionMesh",
			bodyMaterial(base_variant),
			1.0,
			0.16)
	       << '\n'
	       << generatedMeshRule(
			symbol + "Glass",
			mesh_key_prefix + "GlassMesh",
			"smokytransparentglass",
			0.56,
			0.10)
	       << '\n'
	       << VehicleWheelGrammarLoweringService().lowerWheelRule(
			base_variant,
			symbol + "Wheel");
	return stream.str();
}

std::string McsMv21SemanticGrammarLoweringService::lowerVariantRule(
	const ModernCarSemanticVariant &base_variant,
	const McsMv21SemanticVariantDefinition &semantic_variant,
	const std::string &rule_name,
	const std::string &object_identifier,
	const std::string &container_identifier) const
{
	if (base_variant.identifier() != semantic_variant.identifier()) {
		throw std::invalid_argument(
			"MCSMv2.1 grammar lowering requires matching base and semantic variants.");
	}
	const VehiclePackageParameters &package = base_variant.package();
	const VehicleWheelParameters &wheels =
		base_variant.wheelMotion().wheelGeometry();
	const std::string symbol =
		"MCSMv21" + grammarSymbol(base_variant.identifier());
	std::ostringstream stream;
	stream << rule_name << " ->\n"
	       << "[\n"
	       << "    Object(id(" << object_identifier << ")\n"
	       << "           name(" << grammarSymbol(semantic_variant.displayName()) << ")\n"
	       << "           class(ModernCarSemanticVariantV21)\n";
	if (!container_identifier.empty()) {
		stream << "           container(" << container_identifier << ")\n";
	}
	stream << "           taxonomy(Vehicle MCSMv2_1 RegisteredSurface V2)\n"
	       << "           layer(Equipment) mask(Equipment Terrain Temporary))\n"
	       << "    [\n"
	       << "        Interface(id(packageCentre) type(InspectionInterface)\n"
	       << "                  origin(0 0 0) normal(0 1 0) tangent(0 0 1)\n"
	       << "                  region(Point) tolerance(0.001))\n"
	       << "        " << symbol << "FixedBody\n"
	       << "        " << symbol << "PanelRegions\n"
	       << "        " << symbol << "Glass\n"
	       << "        " << symbol << "Wheel(" << number(-wheels.frontTrack() * 0.5)
	       << ' ' << number(package.frontAxleStation()) << " -1)\n"
	       << "        " << symbol << "Wheel(" << number(wheels.frontTrack() * 0.5)
	       << ' ' << number(package.frontAxleStation()) << " 1)\n"
	       << "        " << symbol << "Wheel(" << number(-wheels.rearTrack() * 0.5)
	       << ' ' << number(package.rearAxleStation()) << " -1)\n"
	       << "        " << symbol << "Wheel(" << number(wheels.rearTrack() * 0.5)
	       << ' ' << number(package.rearAxleStation()) << " 1)\n"
	       << "    ]\n"
	       << "]\n";
	return stream.str();
}
