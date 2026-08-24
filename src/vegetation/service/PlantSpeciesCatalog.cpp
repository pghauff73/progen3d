#include "vegetation/service/PlantSpeciesCatalog.h"

#include "geometry/model/BotanicalBladeProfile.h"
#include "vegetation/model/PhyllotaxisMode.h"

#include <algorithm>
#include <cctype>

namespace {

std::string lowercase_copy(const std::string &value)
{
	std::string lowered = value;
	std::transform(
		lowered.begin(), lowered.end(), lowered.begin(),
		[](unsigned char character) {
			return static_cast<char>(std::tolower(character));
		});
	return lowered;
}

const std::vector<PlantSpeciesSpecification> &catalog_species()
{
	static const std::vector<PlantSpeciesSpecification> entries = {
		PlantSpeciesSpecification(
			"DeciduousTree",
			PlantArchitecture::Tree,
			PlantBranchingSpecification(
				3.2f, 0.24f, 0.025f, 7, 2, 2, 2, 43.0f, 137.50776f,
				0.48f, 0.65f, 0.84f, 2.0f, 5.0f, 0.08f, 0.10f),
			PlantLeafSpecification(
				BotanicalBladeProfile::Ovate, 0.12f, 0.055f, 0.004f, 7.0f,
				0.0015f, 1,
				PlantPetioleSpecification(0.025f, 0.0030f, 0.0015f),
				PhyllotaxisSpecification(
					PhyllotaxisMode::Alternate, 137.50776f, 0.08f)),
			8.0f,
			12.0f),
		PlantSpeciesSpecification(
			"Shrub",
			PlantArchitecture::Shrub,
			PlantBranchingSpecification(
				1.35f, 0.095f, 0.012f, 5, 2, 3, 1, 52.0f, 109.0f,
				0.58f, 0.70f, 0.80f, 2.0f, 8.0f, 0.12f, 0.06f),
			PlantLeafSpecification(
				BotanicalBladeProfile::Elliptic, 0.075f, 0.034f, 0.003f, 4.0f,
				0.0010f, 1,
				PlantPetioleSpecification(0.014f, 0.0020f, 0.0010f),
				PlantCompoundLeafSpecification(
					3, 0.080f, 0.0018f, 0.0008f,
					PhyllotaxisMode::Opposite, 180.0f, 0.12f, 0.75f),
				PhyllotaxisSpecification(
					PhyllotaxisMode::Alternate, 137.50776f, 0.12f, 1)),
			2.0f,
			4.0f,
			PlantFlowerSpecification(
				BotanicalBladeProfile::Obovate, 0.050f, 0.027f, 0.012f,
				0.006f, 5.0f, 0.0008f, 0.92f,
				WhorlSpecification(5, 0.012f, 0.0f, 24.0f),
				FlowerHeadSpecification(),
				InflorescenceSpecification(
					InflorescenceKind::Panicle, 2, 0.035f, 0.018f,
					0.0f, 18.0f, 0.96f))),
		PlantSpeciesSpecification(
			"FloweringHerb",
			PlantArchitecture::Herb,
			PlantBranchingSpecification(
				0.72f, 0.026f, 0.004f, 6, 1, 2, 2, 38.0f, 137.50776f,
				0.38f, 0.72f, 0.76f, 2.0f, 3.0f, 0.05f, 0.22f),
			PlantLeafSpecification(
				BotanicalBladeProfile::Lanceolate, 0.095f, 0.028f, 0.006f, 3.0f,
				0.0008f, 0,
				PlantPetioleSpecification(0.018f, 0.0018f, 0.0007f),
				PhyllotaxisSpecification(
					PhyllotaxisMode::Spiral, 137.50776f, 0.065f)),
			0.45f,
			0.75f,
			PlantFlowerSpecification(
				BotanicalBladeProfile::Obovate, 0.090f, 0.045f, 0.030f,
				0.012f, -4.0f, 0.0010f, 0.90f,
				WhorlSpecification(5, 0.018f, 0.0f, 28.0f),
				FlowerHeadSpecification(
					3, 0.045f, 137.50776f, 0.0f, 8.0f, 0.985f))),
		PlantSpeciesSpecification(
			"GrassClump",
			PlantArchitecture::Grass,
			PlantBranchingSpecification(
				0.52f, 0.014f, 0.0025f, 7, 0, 0, 1, 18.0f, 137.50776f,
				0.20f, 0.70f, 0.74f, 2.0f, 7.0f, 0.16f, 0.30f),
			PlantLeafSpecification(
				BotanicalBladeProfile::Linear, 0.34f, 0.018f, 0.045f, 8.0f,
				0.0005f, 0,
				PlantPetioleSpecification(0.006f, 0.0010f, 0.0004f),
				PhyllotaxisSpecification(
					PhyllotaxisMode::Rosette, 32.72727f, 0.025f, 11)),
			0.25f,
			0.50f),
		PlantSpeciesSpecification(
			"IvyVine",
			PlantArchitecture::Vine,
			PlantBranchingSpecification(
				2.4f, 0.032f, 0.004f, 12, 1, 1, 3, 34.0f, 137.50776f,
				0.32f, 0.68f, 0.82f, 2.0f, 6.0f, 0.10f, 0.08f),
			PlantLeafSpecification(
				BotanicalBladeProfile::Palmate, 0.085f, 0.075f, 0.004f, 5.0f,
				0.0009f, 0,
				PlantPetioleSpecification(0.030f, 0.0022f, 0.0010f),
				PhyllotaxisSpecification(
					PhyllotaxisMode::Alternate, 137.50776f, 0.09f)),
			1.0f,
			2.0f),
	};
	return entries;
}

} // namespace

const std::vector<PlantSpeciesSpecification> &PlantSpeciesCatalog::species() const
{
	return catalog_species();
}

const PlantSpeciesSpecification *PlantSpeciesCatalog::find(
	const std::string &identifier) const
{
	const std::string lowered = lowercase_copy(identifier);
	for (const PlantSpeciesSpecification &entry : catalog_species()) {
		if (lowercase_copy(entry.identifier()) == lowered) return &entry;
	}
	return nullptr;
}

std::vector<std::string> PlantSpeciesCatalog::identifiers() const
{
	std::vector<std::string> result;
	result.reserve(catalog_species().size());
	for (const PlantSpeciesSpecification &entry : catalog_species()) {
		result.push_back(entry.identifier());
	}
	return result;
}
