#include "vehicle/mcsmv2/service/McsMv22KinematicCatalogFactory.h"
#include "vehicle/mcsmv2/service/McsMv22KinematicGrammarLoweringService.h"
#include "vehicle/mcsmv2/service/ModernCarSemanticCatalogFactory.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

const McsMv22KinematicVariantDefinition &findKinematicVariant(
	const McsMv22KinematicFamilyDefinition &family,
	const std::string &identifier)
{
	for (const McsMv22KinematicVariantDefinition &variant : family.variants()) {
		if (variant.identifier() == identifier) return variant;
	}
	throw std::runtime_error("MCSMv2.2 variant was not found: " + identifier);
}

void writeFile(const std::filesystem::path &path, const std::string &contents)
{
	std::ofstream stream(path);
	if (!stream) throw std::runtime_error("Could not write " + path.string());
	stream << contents;
}

} // namespace

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cerr << "usage: generate-mcsmv22-grammars OUTPUT_DIRECTORY\n";
		return 2;
	}
	try {
		const std::filesystem::path output_directory(argv[1]);
		std::filesystem::create_directories(output_directory);
		const ModernCarSemanticFamily base_family =
			ModernCarSemanticCatalogFactory().createFamily();
		const McsMv22KinematicFamilyDefinition kinematic_family =
			McsMv22KinematicCatalogFactory().createFamilyDefinition();
		const McsMv22KinematicGrammarLoweringService lowering_service;
		writeFile(
			output_directory / "MCSMv22_Closed_Family_Preview.p3d",
			lowering_service.lowerFamilyClosedPreview(base_family, kinematic_family));
		writeFile(
			output_directory / "MCSMv22_Engineering_Family_Preview.p3d",
			lowering_service.lowerFamilyEngineeringPreview(base_family, kinematic_family));
		for (const ModernCarSemanticVariant &base_variant : base_family.variants()) {
			const McsMv22KinematicVariantDefinition &kinematic_variant =
				findKinematicVariant(kinematic_family, base_variant.identifier());
			writeFile(
				output_directory /
					("MCSMv22_" + base_variant.identifier() + "_Closed_Preview.p3d"),
				lowering_service.lowerVariantClosedPreview(
					base_variant, kinematic_variant));
			writeFile(
				output_directory /
					("MCSMv22_" + base_variant.identifier() + "_Open_Preview.p3d"),
				lowering_service.lowerVariantOpenPreview(
					base_variant, kinematic_variant));
		}
	}
	catch (const std::exception &error) {
		std::cerr << "FAIL: " << error.what() << '\n';
		return 1;
	}
	return 0;
}
