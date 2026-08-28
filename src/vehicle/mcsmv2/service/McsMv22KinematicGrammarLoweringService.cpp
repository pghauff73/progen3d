#include "vehicle/mcsmv2/service/McsMv22KinematicGrammarLoweringService.h"

#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

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

std::string meshKeyPrefix(const std::string &identifier)
{
	if (identifier == "reference") return "MCSMv22Reference";
	if (identifier == "track") return "MCSMv22Track";
	if (identifier == "aero") return "MCSMv22Aero";
	if (identifier == "crossover") return "MCSMv22Crossover";
	throw std::invalid_argument(
		"No MCSMv2.2 generated mesh key is registered for variant '" +
		identifier + "'.");
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

std::string generatedMeshRule(
	const std::string &rule_name,
	const std::string &mesh_key,
	const std::string &material,
	const std::string &alpha,
	const std::string &detail)
{
	std::ostringstream stream;
	stream << rule_name << " ->\n"
	       << "[\n"
	       << "    !I(GeneratedMeshReference(\n"
	       << "          meshKey(" << mesh_key << ")\n"
	       << "          detail(" << detail << ")\n"
	       << "          topology(surface))\n"
	       << "       material(" << material << ") alpha(" << alpha
	       << ") texscale(0.1))\n"
	       << "]\n";
	return stream.str();
}

const McsMv22KinematicVariantDefinition &findKinematicVariant(
	const McsMv22KinematicFamilyDefinition &family,
	const std::string &identifier)
{
	for (const McsMv22KinematicVariantDefinition &variant : family.variants()) {
		if (variant.identifier() == identifier) return variant;
	}
	throw std::invalid_argument(
		"MCSMv2.2 kinematic catalog does not contain variant '" + identifier + "'.");
}

} // namespace

std::string McsMv22KinematicGrammarLoweringService::lowerVariantClosedPreview(
	const ModernCarSemanticVariant &base_variant,
	const McsMv22KinematicVariantDefinition &kinematic_variant) const
{
	const std::string symbol =
		"MCSMv22" + grammarSymbol(base_variant.identifier()) + "Closed";
	std::ostringstream stream;
	stream << "# MCSMv2.2 executable closed-state preview.\n"
	       << "# +X right, +Y up, +Z forward; origin is ground package centre.\n\n"
	       << "Start ->\n"
	       << "    " << symbol << "Preview\n\n"
	       << lowerVariantRule(
		       base_variant, kinematic_variant, symbol + "Preview",
		       objectIdentifier("MCSMV22_" + base_variant.identifier() + "_CLOSED"),
		       false);
	return stream.str();
}

std::string McsMv22KinematicGrammarLoweringService::lowerVariantOpenPreview(
	const ModernCarSemanticVariant &base_variant,
	const McsMv22KinematicVariantDefinition &kinematic_variant) const
{
	const std::string symbol =
		"MCSMv22" + grammarSymbol(base_variant.identifier()) + "Open";
	std::ostringstream stream;
	stream << "# MCSMv2.2 executable open-state kinematics preview.\n"
	       << "# +X right, +Y up, +Z forward; origin is ground package centre.\n\n"
	       << "Start ->\n"
	       << "    " << symbol << "Preview\n\n"
	       << lowerVariantRule(
		       base_variant, kinematic_variant, symbol + "Preview",
		       objectIdentifier("MCSMV22_" + base_variant.identifier() + "_OPEN"),
		       true);
	return stream.str();
}

std::string McsMv22KinematicGrammarLoweringService::lowerFamilyClosedPreview(
	const ModernCarSemanticFamily &base_family,
	const McsMv22KinematicFamilyDefinition &kinematic_family) const
{
	std::ostringstream stream;
	stream << "# MCSMv2.2 executable closed family preview.\n\n"
	       << "Start ->\n"
	       << "    MCSMv22ClosedFamilyPreview\n\n"
	       << "MCSMv22ClosedFamilyPreview ->\n"
	       << "[\n"
	       << "    Object(id(MCSMV22_CLOSED_FAMILY) name(McsMv22ClosedFamily)\n"
	       << "           class(ModernCarKinematicFamilyV22)\n"
	       << "           taxonomy(Vehicle MCSMv2_2 RegisteredSurface SampledKinematics V3 Family)\n"
	       << "           layer(Equipment) mask(Equipment Terrain Temporary))\n"
	       << "    [\n";
	const double positions[] = {-7.2, -2.4, 2.4, 7.2};
	for (std::size_t index = 0u; index < base_family.variants().size(); ++index) {
		stream << "        [ T(" << positions[index] << " 0 0) MCSMv22"
		       << grammarSymbol(base_family.variants()[index].identifier())
		       << "FamilyClosed ]\n";
	}
	stream << "    ]\n"
	       << "]\n\n";
	for (const ModernCarSemanticVariant &base_variant : base_family.variants()) {
		stream << lowerVariantRule(
			base_variant,
			findKinematicVariant(kinematic_family, base_variant.identifier()),
			"MCSMv22" + grammarSymbol(base_variant.identifier()) + "FamilyClosed",
			objectIdentifier("MCSMV22_FAMILY_" + base_variant.identifier()), false)
		       << "\n";
	}
	return stream.str();
}

std::string McsMv22KinematicGrammarLoweringService::lowerFamilyEngineeringPreview(
	const ModernCarSemanticFamily &base_family,
	const McsMv22KinematicFamilyDefinition &kinematic_family) const
{
	std::ostringstream stream;
	stream << "# MCSMv2.2 executable engineering family preview.\n\n"
	       << "Start ->\n"
	       << "    MCSMv22EngineeringFamilyPreview\n\n"
	       << "MCSMv22EngineeringFamilyPreview ->\n"
	       << "[\n"
	       << "    Object(id(MCSMV22_ENGINEERING_FAMILY) name(McsMv22EngineeringFamily)\n"
	       << "           class(ModernCarKinematicFamilyV22)\n"
	       << "           taxonomy(Vehicle MCSMv2_2 EngineeringPreview SampledKinematics V3 Family)\n"
	       << "           layer(Equipment) mask(Equipment Terrain Temporary))\n"
	       << "    [\n";
	const double positions[] = {-7.2, -2.4, 2.4, 7.2};
	for (std::size_t index = 0u; index < base_family.variants().size(); ++index) {
		stream << "        [ T(" << positions[index] << " 0 0) MCSMv22"
		       << grammarSymbol(base_family.variants()[index].identifier())
		       << "FamilyEngineering ]\n";
	}
	stream << "    ]\n"
	       << "]\n\n";
	for (const ModernCarSemanticVariant &base_variant : base_family.variants()) {
		stream << lowerVariantRule(
			base_variant,
			findKinematicVariant(kinematic_family, base_variant.identifier()),
			"MCSMv22" + grammarSymbol(base_variant.identifier()) +
				"FamilyEngineering",
			objectIdentifier("MCSMV22_ENGINEERING_" + base_variant.identifier()), true)
		       << "\n";
	}
	return stream.str();
}

std::string McsMv22KinematicGrammarLoweringService::lowerVariantRule(
	const ModernCarSemanticVariant &base_variant,
	const McsMv22KinematicVariantDefinition &kinematic_variant,
	const std::string &rule_name,
	const std::string &object_identifier,
	bool open_state) const
{
	if (base_variant.identifier() != kinematic_variant.identifier()) {
		throw std::invalid_argument(
			"MCSMv2 and MCSMv2.2 variant identifiers must match during lowering.");
	}
	const std::string prefix = meshKeyPrefix(base_variant.identifier());
	const std::string component_prefix = rule_name + "Component";
	const std::string closure_state = open_state ? "Open" : "Closed";
	const std::string tyre_component = open_state ? "TyreSweep" : "NominalTyre";
	std::ostringstream stream;
	stream << rule_name << " ->\n"
	       << "[\n"
	       << "    Object(id(" << object_identifier << ") name("
	       << grammarSymbol(base_variant.displayName()) << ")\n"
	       << "           class(ModernCarKinematicVariantV22)\n"
	       << "           taxonomy(Vehicle MCSMv2_2 RegisteredSurface SampledKinematics V3)\n"
	       << "           layer(Equipment) mask(Equipment Terrain Temporary))\n"
	       << "    [\n"
	       << "        Interface(id(packageCentre) type(InspectionInterface)\n"
	       << "                  origin(0 0 0) normal(0 1 0) tangent(0 0 1)\n"
	       << "                  region(Point) tolerance(0.001))\n"
	       << "        " << component_prefix << "FixedBody\n"
	       << "        " << component_prefix << "Closures\n"
	       << "        " << component_prefix << "Glass\n"
	       << "        " << component_prefix << "Suspension\n"
	       << "        " << component_prefix << "Tyres\n"
	       << "    ]\n"
	       << "]\n\n"
	       << generatedMeshRule(
		       component_prefix + "FixedBody", prefix + "FixedBodyMesh",
		       bodyMaterial(base_variant), "1", "LOD2")
	       << "\n"
	       << generatedMeshRule(
		       component_prefix + "Closures",
		       prefix + "Closures" + closure_state + "Mesh",
		       bodyMaterial(base_variant), "1", "LOD2")
	       << "\n"
	       << generatedMeshRule(
		       component_prefix + "Glass", prefix + "Glass" + closure_state + "Mesh",
		       "smokytransparentglass", "0.56", "LOD2")
	       << "\n"
	       << generatedMeshRule(
		       component_prefix + "Suspension", prefix + "SuspensionMesh",
		       "charcoalroughmetal", "1", "LOD1")
	       << "\n"
	       << generatedMeshRule(
		       component_prefix + "Tyres", prefix + tyre_component + "Mesh",
		       "charcoalroughrubber", open_state ? "0.42" : "1", "LOD1");
	return stream.str();
}
