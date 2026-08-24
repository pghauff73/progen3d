#include "chair/service/ChairCatalog.h"

#include <algorithm>

namespace {

ChairDesignDefinition design(
	const char *identifier,
	const char *name,
	std::vector<ChairUseContext> contexts,
	float width,
	float depth,
	float seat_height,
	float overall_height,
	ChairSupportKind support,
	float member_radius,
	float camber,
	float thickness,
	float recline,
	bool back,
	bool arms,
	bool cushion,
	bool footrest,
	std::vector<std::string> materials)
{
	return ChairDesignDefinition(
		identifier, name,
		ChairContextApplicabilityRelationship(std::move(contexts)),
		ChairDimensionSpecification(width, depth, seat_height, overall_height),
		ChairErgonomicSpecification(2.0f, recline, 0.18f),
		ChairGeometryParameterSet(
			support, member_radius, camber, thickness,
			back, arms, cushion, footrest),
		ChairMaterialVariantSet(std::move(materials)));
}

}

ChairCatalog::ChairCatalog()
	: designs_{
		design("COMv1.Windsor", "Windsor spindle-back", {ChairUseContext::Dining, ChairUseContext::Kitchen}, 0.48f, 0.50f, 0.45f, 0.91f, ChairSupportKind::FourLeg, 0.021f, 0.018f, 0.020f, 7.0f, true, false, false, false, {"Oak", "PaintedWood"}),
		design("COMv1.ShakerLadderBack", "Shaker ladder-back", {ChairUseContext::Dining, ChairUseContext::Kitchen}, 0.47f, 0.49f, 0.45f, 0.96f, ChairSupportKind::FourLeg, 0.023f, 0.012f, 0.020f, 5.0f, true, false, false, false, {"Maple", "Ash"}),
		design("COMv1.BentwoodCafe", "Bentwood cafe", {ChairUseContext::Dining, ChairUseContext::Kitchen}, 0.46f, 0.49f, 0.46f, 0.88f, ChairSupportKind::FourLeg, 0.019f, 0.028f, 0.018f, 10.0f, true, false, false, false, {"Beech", "BlackLacquer"}),
		design("COMv1.Parsons", "Upholstered Parsons", {ChairUseContext::Dining, ChairUseContext::Living}, 0.50f, 0.56f, 0.47f, 0.98f, ChairSupportKind::FourLeg, 0.032f, 0.020f, 0.055f, 6.0f, true, false, true, false, {"Fabric", "Leather"}),
		design("COMv1.ScandinavianShell", "Molded Scandinavian shell", {ChairUseContext::Dining, ChairUseContext::Kitchen, ChairUseContext::Study, ChairUseContext::Patio}, 0.52f, 0.54f, 0.46f, 0.84f, ChairSupportKind::FourLeg, 0.018f, 0.045f, 0.022f, 12.0f, true, false, false, false, {"MoldedPlywood", "Polypropylene"}),
		design("COMv1.Wishbone", "Wishbone Y-chair", {ChairUseContext::Dining, ChairUseContext::Living}, 0.55f, 0.53f, 0.45f, 0.76f, ChairSupportKind::FourLeg, 0.020f, 0.030f, 0.020f, 14.0f, true, true, false, false, {"Oak", "PaperCord"}),
		design("COMv1.Cantilever", "Tubular cantilever", {ChairUseContext::Dining, ChairUseContext::Study}, 0.50f, 0.56f, 0.46f, 0.83f, ChairSupportKind::Cantilever, 0.017f, 0.025f, 0.030f, 11.0f, true, false, true, false, {"Chrome", "Leather"}),
		design("COMv1.BackedCounterStool", "Backed counter stool", {ChairUseContext::Kitchen}, 0.44f, 0.46f, 0.66f, 0.96f, ChairSupportKind::FourLeg, 0.022f, 0.018f, 0.025f, 8.0f, true, false, false, true, {"Timber", "PowderCoat"}),
		design("COMv1.SaddleStool", "Saddle stool", {ChairUseContext::Kitchen}, 0.45f, 0.34f, 0.67f, 0.72f, ChairSupportKind::Pedestal, 0.028f, 0.055f, 0.035f, 0.0f, false, false, true, true, {"Timber", "Leather"}),
		design("COMv1.MeshTask", "Mesh task chair", {ChairUseContext::Study}, 0.62f, 0.62f, 0.47f, 1.02f, ChairSupportKind::SwivelCaster, 0.018f, 0.030f, 0.020f, 14.0f, true, true, true, false, {"Mesh", "Nylon"}),
		design("COMv1.ExecutiveHighBack", "Executive high-back", {ChairUseContext::Study}, 0.68f, 0.70f, 0.50f, 1.20f, ChairSupportKind::SwivelCaster, 0.021f, 0.035f, 0.065f, 15.0f, true, true, true, false, {"Leather", "Aluminium"}),
		design("COMv1.LoungeArmchair", "Lounge armchair", {ChairUseContext::Living}, 0.78f, 0.78f, 0.43f, 0.86f, ChairSupportKind::FourLeg, 0.034f, 0.050f, 0.085f, 18.0f, true, true, true, false, {"Fabric", "Leather"}),
		design("COMv1.MidCenturyAccent", "Mid-century accent", {ChairUseContext::Living, ChairUseContext::Study}, 0.64f, 0.68f, 0.43f, 0.82f, ChairSupportKind::FourLeg, 0.028f, 0.038f, 0.050f, 16.0f, true, true, true, false, {"Walnut", "Fabric"}),
		design("COMv1.Adirondack", "Adirondack", {ChairUseContext::Patio}, 0.76f, 0.86f, 0.36f, 0.96f, ChairSupportKind::Sled, 0.035f, 0.025f, 0.030f, 24.0f, true, true, false, false, {"Cedar", "RecycledPlastic"}),
		design("COMv1.MetalBistro", "Metal bistro", {ChairUseContext::Patio, ChairUseContext::Kitchen}, 0.45f, 0.48f, 0.45f, 0.84f, ChairSupportKind::Sled, 0.014f, 0.015f, 0.012f, 8.0f, true, false, false, false, {"PowderCoatSteel", "GalvanizedSteel"})}
{
}

const ChairDesignDefinition *ChairCatalog::findByIdentifier(
	const std::string &identifier) const
{
	const auto found = std::find_if(
		designs_.begin(), designs_.end(),
		[&identifier](const ChairDesignDefinition &definition) {
			return definition.identifier() == identifier;
		});
	return found == designs_.end() ? nullptr : &*found;
}

std::vector<const ChairDesignDefinition *> ChairCatalog::designsForContext(
	ChairUseContext context) const
{
	std::vector<const ChairDesignDefinition *> matches;
	for (const ChairDesignDefinition &definition : designs_) {
		if (definition.applicability().appliesTo(context)) matches.push_back(&definition);
	}
	return matches;
}
